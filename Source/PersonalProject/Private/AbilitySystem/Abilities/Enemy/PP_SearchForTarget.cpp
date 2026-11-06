

#include "AbilitySystem/Abilities/Enemy/PP_SearchForTarget.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/AbilityTasks/PP_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AI/UPP_AIPerceptionComponent.h"
#include "Characters/PP_EnemyCharacter.h"
#include "Framework/PP_Log.h"
#include "GameplayTags/PPTags.h"
#include "Tasks/AITask_MoveTo.h"
#include "TimerManager.h"
#include "Utils/PP_BlueprintLibrary.h"

UPP_SearchForTarget::UPP_SearchForTarget()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UPP_SearchForTarget::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UWorld* World = GetWorld();
	AActor* Avatar = GetAvatarActorFromActorInfo();
	UE_LOG(LogPP, Log, TEXT("[SearchForTarget] ActivateAbility called. Avatar=%s, World=%s, IsServer=%d"),
		Avatar ? *Avatar->GetName() : TEXT("null"),
		World ? *World->GetName() : TEXT("null"),
		Avatar && Avatar->HasAuthority());

	OwningEnemy = Cast<APP_EnemyCharacter>(Avatar);
	if (!OwningEnemy.IsValid())
	{
		UE_LOG(LogPP, Error, TEXT("[SearchForTarget] OwningEnemy 无效，Ability 结束"));
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	OwningAIController = Cast<AAIController>(OwningEnemy->GetController());
	if (!OwningAIController.IsValid())
	{
		// AIController 可能还没 Possess（BeginPlay 时序），等下一帧重试
		UE_LOG(LogPP, Warning, TEXT("[SearchForTarget] AIController 未就位（Enemy=%s, Controller=%s），下一帧重试"),
			*OwningEnemy->GetName(),
			OwningEnemy->GetController() ? *OwningEnemy->GetController()->GetName() : TEXT("null"));
		if (World)
		{
			World->GetTimerManager().SetTimerForNextTick([this, Handle, ActorInfo, ActivationInfo]()
			{
				OwningAIController = Cast<AAIController>(OwningEnemy->GetController());
				if (OwningAIController.IsValid())
				{
					UE_LOG(LogPP, Log, TEXT("[SearchForTarget] 重试成功：AIController=%s"), *OwningAIController->GetName());
					ActivateAbility(Handle, ActorInfo, ActivationInfo, nullptr);
				}
				else
				{
					UE_LOG(LogPP, Error, TEXT("[SearchForTarget] 重试仍失败，Ability 结束"));
					EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
				}
			});
		}
		return;
	}

	UE_LOG(LogPP, Log, TEXT("[SearchForTarget] 启动搜索循环. Enemy=%s, AIController=%s"),
		*OwningEnemy->GetName(), *OwningAIController->GetName());
	StartSearch();

	// 监听 HitReact（被打时取消待发攻击并立即重新索敌）
	WaitHitReactTask = UPP_WaitGameplayEvent::WaitGameplayEventToActorProxy(
		GetAvatarActorFromActorInfo(), PPTags::Events::Enemy::HitReact);
	WaitHitReactTask->EventReceived.AddDynamic(this, &ThisClass::HitReactEventReceived);
	WaitHitReactTask->StartActivation();
}

void UPP_SearchForTarget::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility,
	bool bWasCancelled)
{
	// UAbilityTask 自带生命周期管理，不需要手动清理 TimerManager
	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UPP_SearchForTarget::HitReactEventReceived(FGameplayEventData Payload)
{
	if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Orange, TEXT("[SearchForTarget] HitReact → 取消待发攻击 + StartSearch"));

	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s 收到 HitReact 事件：SearchPending=%d MoveToPending=%d"),
		OwningEnemy.IsValid() ? *OwningEnemy->GetName() : TEXT("null"), SearchPending, MoveToPending);

	// 取消待发攻击的 Delay（如果还没触发 Attack）
	if (AttackDelayTask)
	{
		AttackDelayTask->EndTask();
		AttackDelayTask = nullptr;
	}
	// 注意：不取消 PostAttackCooldownTask，冷却是唯一的节奏来源，打断不能把它清掉
	// 取消 MoveTo 任务
	if (MoveToLocationOrActorTask)
	{
		MoveToLocationOrActorTask->EndTask();
		MoveToLocationOrActorTask = nullptr;
		if (MoveToPending > 0) --MoveToPending;
	}

	// 立即重新索敌
	StartSearch();
}

void UPP_SearchForTarget::StartSearch()
{
	if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("UPP_SearchForTarget::StartSearch"));
	if (!OwningEnemy.IsValid()) return;

	// 幂等：已有搜索挂起时忽略，避免 HitReact / 冷却 / 多点调用叠出多条循环
	if (SearchPending > 0)
	{
		UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s StartSearch 被忽略：已有挂起搜索（挂起=%d）"),
			*OwningEnemy->GetName(), SearchPending);
		return;
	}

	const float SearchDelay = FMath::RandRange(OwningEnemy->MinAttackDelay, OwningEnemy->MaxAttackDelay);
	++SearchPending;
	++SearchSerial;
	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s StartSearch #%d 延迟=%.2f 挂起搜索=%d"),
		*OwningEnemy->GetName(), SearchSerial, SearchDelay, SearchPending);
	SearchDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SearchDelay);
	SearchDelayTask->OnFinish.AddDynamic(this, &ThisClass::Search);
	SearchDelayTask->Activate();
}

void UPP_SearchForTarget::Search()
{
	if (!OwningEnemy.IsValid()) return;

	if (SearchPending > 0) --SearchPending;
	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s Search 到时 挂起搜索=%d"), *OwningEnemy->GetName(), SearchPending);

	// === 改由 AIPerceptionComponent 提供目标（不再自己算距离）===
	UPP_AIPerceptionComponent* Perception = OwningEnemy->GetAIPerceptionComponent();
	if (!Perception)
	{
		UE_LOG(LogPP, Warning, TEXT("[SearchForTarget] %s: GetAIPerceptionComponent() 返回 null，敌人不会动！"),
			*OwningEnemy->GetName());
		StartSearch();
		return;
	}
	APP_BaseCharacter* Target = Perception->CurrentTarget.Get();

	if (!IsValid(Target) || !Target->IsAlive())
	{
		UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s Search 无目标 → 下一轮"), *OwningEnemy->GetName());
		if (OwningAIController.IsValid())
		{
			OwningAIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
		StartSearch();
		return;
	}

	if (OwningAIController.IsValid())
	{
		OwningAIController->SetFocus(Target);
	}

	UE_LOG(LogPP, Log, TEXT("[SearchForTarget] → MoveTo: 目标=%s, Acceptance=%.0f"),
		*Target->GetName(), OwningEnemy->AcceptanceRadius);
	MoveToTargetAndAttack();
}

void UPP_SearchForTarget::MoveToTargetAndAttack()
{
	if (!OwningEnemy.IsValid() || !OwningAIController.IsValid()) return;
	if (!OwningEnemy->IsAlive())
	{
		StartSearch();
		return;
	}

	// 从 Perception 读当前目标
	UPP_AIPerceptionComponent* Perception = OwningEnemy->GetAIPerceptionComponent();
	APP_BaseCharacter* Target = Perception ? Perception->CurrentTarget.Get() : nullptr;
	if (!IsValid(Target))
	{
		StartSearch();
		return;
	}

	MoveToLocationOrActorTask = UAITask_MoveTo::AIMoveTo(
		OwningAIController.Get(),
		FVector(),
		Target,
		OwningEnemy->AcceptanceRadius);

	++MoveToPending;
	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s MoveTo 开始 目标=%s 距离=%.0f 并发MoveTo=%d"),
		*OwningEnemy->GetName(), *Target->GetName(),
		FVector::Dist(OwningEnemy->GetActorLocation(), Target->GetActorLocation()), MoveToPending);

	MoveToLocationOrActorTask->OnMoveTaskFinished.AddUObject(this, &ThisClass::AttackTarget);
	MoveToLocationOrActorTask->ConditionalPerformMove();
}

void UPP_SearchForTarget::AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController)
{
	if (MoveToPending > 0) --MoveToPending;
	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s MoveTo 结束 Result=%d 剩余并发MoveTo=%d"),
		OwningEnemy.IsValid() ? *OwningEnemy->GetName() : TEXT("null"), static_cast<int32>(Result.GetValue()), MoveToPending);

	if (Result != EPathFollowingResult::Success)
	{
		StartSearch();
		return;
	}

	// 再确认一次目标还在
	UPP_AIPerceptionComponent* Perception = OwningEnemy.IsValid() ? OwningEnemy->GetAIPerceptionComponent() : nullptr;
	APP_BaseCharacter* Target = Perception ? Perception->CurrentTarget.Get() : nullptr;
	if (!IsValid(Target))
	{
		StartSearch();
		return;
	}

	OwningEnemy->RotateToTarget(Target);
	
	// 原攻击前摇
	AttackDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, OwningEnemy->GetTimeLiveLenght());
	AttackDelayTask->OnFinish.AddDynamic(this, &ThisClass::Attack);
	AttackDelayTask->Activate();
}

void UPP_SearchForTarget::Attack()
{
	if (!OwningEnemy.IsValid()) return;

	UWorld* World = OwningEnemy->GetWorld();
	const float Now = World ? World->GetTimeSeconds() : 0.f;

	// 冷却门控：冷却未结束时一律不出手（冷却任务是唯一的节奏来源）
	if (World && Now < NextAttackAllowedTime)
	{
		UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s 冷却中（剩余 %.2fs），跳过本次攻击"),
			*OwningEnemy->GetName(), NextAttackAllowedTime - Now);
		StartSearch();
		return;
	}

	const FGameplayTag AttackTag = PPTags::PPAbilities::Enemy::Attack;
	const bool bActivated = GetAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesByTag(AttackTag.GetSingleTagContainer());

	const float Cooldown = FMath::RandRange(OwningEnemy->AttackCooldownMin, OwningEnemy->AttackCooldownMax);
	NextAttackAllowedTime = Now + Cooldown;

	const float Delta = (LastAttackTime >= 0.f) ? (Now - LastAttackTime) : -1.f;
	LastAttackTime = Now;
	++AttackSerial;

	UPP_AIPerceptionComponent* Perception = OwningEnemy->GetAIPerceptionComponent();
	APP_BaseCharacter* Target = Perception ? Perception->CurrentTarget.Get() : nullptr;
	const float Dist = (Target && IsValid(Target))
		? FVector::Dist(OwningEnemy->GetActorLocation(), Target->GetActorLocation())
		: -1.f;

	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s Attack #%d 距上次=%.2fs 目标距离=%.0f 前摇=%.2fs 激活=%d 挂起搜索=%d 并发MoveTo=%d"),
		*OwningEnemy->GetName(), AttackSerial, Delta, Dist,
		OwningEnemy->GetTimeLiveLenght(), bActivated ? 1 : 0, SearchPending, MoveToPending);

	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s 进入攻击冷却 %.2fs"), *OwningEnemy->GetName(), Cooldown);
	if (PostAttackCooldownTask)
	{
		PostAttackCooldownTask->EndTask();
		PostAttackCooldownTask = nullptr;
	}
	PostAttackCooldownTask = UAbilityTask_WaitDelay::WaitDelay(this, Cooldown);
	PostAttackCooldownTask->OnFinish.AddDynamic(this, &ThisClass::OnPostAttackCooldown);
	PostAttackCooldownTask->Activate();
}

void UPP_SearchForTarget::OnPostAttackCooldown()
{
	UE_LOG(LogPP, Log, TEXT("[EnemyAttack] %s 冷却结束 → 下一轮搜索（挂起搜索=%d 并发MoveTo=%d）"),
		OwningEnemy.IsValid() ? *OwningEnemy->GetName() : TEXT("null"), SearchPending, MoveToPending);
	if (PostAttackCooldownTask)
	{
		PostAttackCooldownTask = nullptr;
	}
	StartSearch();
}
