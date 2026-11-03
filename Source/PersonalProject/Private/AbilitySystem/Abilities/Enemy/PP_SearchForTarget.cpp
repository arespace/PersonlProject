// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Enemy/PP_SearchForTarget.h"

#include "AbilitySystemComponent.h"
#include "AIController.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "AbilitySystem/AbilityTasks/PP_WaitGameplayEvent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "AI/UPP_AIPerceptionComponent.h"
#include "Characters/PP_EnemyCharacter.h"
#include "GameplayTags/PPTags.h"
#include "Tasks/AITask_MoveTo.h"
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
	
	OwningEnemy = Cast<APP_EnemyCharacter>(GetAvatarActorFromActorInfo());
	check(OwningEnemy.IsValid());
	OwningAIController = Cast<AAIController>(OwningEnemy->GetController());
	check(OwningAIController.IsValid());
	
	StartSearch();

	// 监听 EndAttack（原有的：攻击蒙太奇结束后重新搜索）
	WaitEndAttackTask = UPP_WaitGameplayEvent::WaitGameplayEventToActorProxy(
		GetAvatarActorFromActorInfo(), PPTags::Events::Enemy::EndAttack);
	WaitEndAttackTask->EventReceived.AddDynamic(this, &ThisClass::EndAttackEventReceived);
	WaitEndAttackTask->StartActivation();

	// 新增：监听 HitReact（被打时取消待发攻击并立即重新索敌）
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

	// 取消待发攻击的 Delay（如果还没触发 Attack）
	if (AttackDelayTask)
	{
		AttackDelayTask->EndTask();
		AttackDelayTask = nullptr;
	}
	if (PostAttackCooldownTask)
	{
		PostAttackCooldownTask->EndTask();
		PostAttackCooldownTask = nullptr;
	}
	// 取消 MoveTo 任务
	if (MoveToLocationOrActorTask)
	{
		MoveToLocationOrActorTask->EndTask();
		MoveToLocationOrActorTask = nullptr;
	}

	// 立即重新索敌
	StartSearch();
}

void UPP_SearchForTarget::StartSearch()
{
	if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("UPP_SearchForTarget::StartSearch"));
	if (!OwningEnemy.IsValid()) return;
	
	const float SearchDelay = FMath::RandRange(OwningEnemy->MinAttackDelay, OwningEnemy->MaxAttackDelay);
	SearchDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, SearchDelay);
	SearchDelayTask->OnFinish.AddDynamic(this, &ThisClass::Search);
	SearchDelayTask->Activate();
}

void UPP_SearchForTarget::EndAttackEventReceived(FGameplayEventData Payload)
{
	if (OwningEnemy.IsValid() && !OwningEnemy->bIsBeingLaunched)
	{
		StartSearch();
	}
}

void UPP_SearchForTarget::Search()
{
	if (!OwningEnemy.IsValid()) return;

	// === 改由 AIPerceptionComponent 提供目标（不再自己算距离）===
	UPP_AIPerceptionComponent* Perception = OwningEnemy->GetAIPerceptionComponent();
	APP_BaseCharacter* Target = Perception ? Perception->CurrentTarget.Get() : nullptr;

	if (!IsValid(Target) || !Target->IsAlive())
	{
		if (OwningAIController.IsValid())
		{
			OwningAIController->ClearFocus(EAIFocusPriority::Gameplay);
		}
		StartSearch();
		return;
	}

	// 直接让 AIController 重新对齐目标（Perception 的 Search 会定期刷新，但我们每次自己触发一次 SetFocus 避免 Tick 空转）
	if (OwningAIController.IsValid())
	{
		OwningAIController->SetFocus(Target);
	}

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

	MoveToLocationOrActorTask->OnMoveTaskFinished.AddUObject(this, &ThisClass::AttackTarget);
	MoveToLocationOrActorTask->ConditionalPerformMove();
}

void UPP_SearchForTarget::AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController)
{
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
	const FGameplayTag AttackTag = PPTags::PPAbilities::Enemy::Attack;
	GetAbilitySystemComponentFromActorInfo()->TryActivateAbilitiesByTag(AttackTag.GetSingleTagContainer());

	// === 攻击发完后进入显式冷却（替代"蒙太奇结束→StartSearch"的紧耦合）===
	if (!OwningEnemy.IsValid()) return;

	const float Cooldown = FMath::RandRange(OwningEnemy->AttackCooldownMin, OwningEnemy->AttackCooldownMax);
	PostAttackCooldownTask = UAbilityTask_WaitDelay::WaitDelay(this, Cooldown);
	PostAttackCooldownTask->OnFinish.AddDynamic(this, &ThisClass::OnPostAttackCooldown);
	PostAttackCooldownTask->Activate();
}

void UPP_SearchForTarget::OnPostAttackCooldown()
{
	if (PostAttackCooldownTask)
	{
		PostAttackCooldownTask = nullptr;
	}
	StartSearch();
}
