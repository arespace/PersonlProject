
#include "Characters/PP_EnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/PP_AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "AI/UPP_AIPerceptionComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/PPTags.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Interfaces/PP_InteractableInterface.h"
#include "Player/PP_PlayerState.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "TimerManager.h"
#include "Net/UnrealNetwork.h"
#include "Quest/PP_QuestEventBridge.h"
#include "Game/PP_CrashGameMode.h"
#include "Game/PP_EnemyRespawnManager.h"
#include "Framework/PP_Log.h"


APP_EnemyCharacter::APP_EnemyCharacter()
{
	// ALS 需要 Tick，不得禁用 PrimaryActorTick

	AbilitySystemComponent = CreateDefaultSubobject<UPP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UPP_AttributeSet>("AttributeSet");
}

void APP_EnemyCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ThisClass,bIsBeingLaunched);
}

UAbilitySystemComponent* APP_EnemyCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

UAttributeSet* APP_EnemyCharacter::GetAttributeSet() const
{
	return AttributeSet;
}

UPP_AIPerceptionComponent* APP_EnemyCharacter::GetAIPerceptionComponent() const
{
	if (const AAIController* AIC = Cast<AAIController>(GetController()))
	{
		return AIC->FindComponentByClass<UPP_AIPerceptionComponent>();
	}
	return nullptr;
}

void APP_EnemyCharacter::StopMovementUntilLanded()
{
	bIsBeingLaunched = true;
	AAIController* AIController = GetController<AAIController>();
	if (!IsValid(AIController)) return;
	AIController->StopMovement();
	if (!LandedDelegate.IsAlreadyBound(this, &ThisClass::EnableMovementOnLanded))
	{
		LandedDelegate.AddDynamic(this, &ThisClass::EnableMovementOnLanded);
	}
}

void APP_EnemyCharacter::EnableMovementOnLanded(const FHitResult& Hit)
{
	bIsBeingLaunched = false;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, PPTags::Events::Enemy::EndAttack, FGameplayEventData());
	LandedDelegate.RemoveAll(this);
}

void APP_EnemyCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 缓存生成点，ResetToSpawn 复位用
	SpawnLocation = GetActorLocation();
	SpawnRotation = GetActorRotation();

	// 蓝图未给 AIController 挂感知组件时运行时补建，避免敌人彻底失去索敌能力
	if (AAIController* AIController = Cast<AAIController>(GetController()))
	{
		if (!AIController->FindComponentByClass<UPP_AIPerceptionComponent>())
		{
			UPP_AIPerceptionComponent* PerceptionComponent =
				NewObject<UPP_AIPerceptionComponent>(AIController, TEXT("PPAIPerception"));
			PerceptionComponent->RegisterComponent();
			PerceptionComponent->StartSearching();
			UE_LOG(LogPP, Log, TEXT("[Enemy] %s 的 AIController 未配置 UPP_AIPerceptionComponent，已自动补建"),
				*GetName());
		}
	}

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this,this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());

	if (!HasAuthority()) return;

	// 注册到重生管理器：动态生成的敌人也能被篝火休息重置
	if (UWorld* World = GetWorld())
	{
		if (APP_CrashGameMode* GM = World->GetAuthGameMode<APP_CrashGameMode>())
		{
			if (UPP_EnemyRespawnManager* Manager = GM->GetEnemyRespawnManager())
			{
				Manager->RegisterEnemy(this);
			}
		}
	}

	GiveStartupAbilities();
	InitializeAttributes();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void APP_EnemyCharacter::ResetToSpawn_Implementation()
{
	UE_LOG(LogPP, Log, TEXT("[Enemy] ResetToSpawn: %s → 位置=(%.0f, %.0f, %.0f)"),
		*GetName(), SpawnLocation.X, SpawnLocation.Y, SpawnLocation.Z);

	// 1. 回到生成点
	SetActorLocation(SpawnLocation, false, nullptr, ETeleportType::TeleportPhysics);
	SetActorRotation(SpawnRotation);

	// 1.5 取消死亡流程留下的延迟销毁并恢复可见/碰撞
	// 死亡 GA 会 SetLifeSpan(1.0) + 隐藏；若篝火在 1 秒内重置，敌人必须撤回销毁计时，
	// 否则会被"复活"后随即销毁。
	SetLifeSpan(0.f);
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	// 2. 恢复 Alive
	SetAlive(true);

	// 3. 恢复碰撞（死亡时关掉了）
	if (auto* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}
	if (auto* CM = GetCharacterMovement())
	{
		CM->SetMovementMode(MOVE_Walking);
	}

	// 4. 清掉死亡状态的残留 GE / 标签
	if (auto* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));
	}

	// 5. 重置属性到满值
	ResetAttributes();

	// 6. 清 AI 状态：停移动、清 Focus、清 Perception
	if (auto* AIC = Cast<AAIController>(GetController()))
	{
		AIC->StopMovement();
		AIC->ClearFocus(EAIFocusPriority::Gameplay);

		if (auto* Perception = AIC->FindComponentByClass<UPP_AIPerceptionComponent>())
		{
			Perception->ClearTarget();
		}
	}

	// 7. 清 Ragdoll（若还在倒地状态）
	StopRagdolling();

	// 8. 清掉死亡流程留下的计时器与击飞标记，并通知索敌 GA 恢复循环
	// 死亡时若挂过自动复活计时器，不清理会在复活后再次触发 HandleDeath。
	GetWorldTimerManager().ClearTimer(RespawnTimerHandle);
	bIsBeingLaunched = false;
	LandedDelegate.RemoveAll(this);
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, PPTags::Events::Enemy::EndAttack, FGameplayEventData());
}

void APP_EnemyCharacter::HandleDeath()
{
	Super::HandleDeath();

	AAIController* AIController = GetController<AAIController>();
	if (IsValid(AIController))
	{
		AIController->StopMovement();
	}

	if (!HasAuthority())
	{
		return;
	}

	// 查找击杀者：优先 GAS Instigator，失败则 fallback 取第一个 PlayerController（单人模式足够）
	AController* KillerController = GetInstigatorController();
	APlayerController* KillerPC = Cast<APlayerController>(KillerController);

	if (!KillerPC)
	{
		UWorld* World = GetWorld();
		if (World)
		{
			for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
			{
				KillerPC = It->Get();
				if (KillerPC) break;
			}
		}
	}

	// ★ 任务系统击杀通知 ★
	if (KillerPC && !EnemyID.IsNone())
	{
		APP_CrashGameMode* GM = GetWorld()->GetAuthGameMode<APP_CrashGameMode>();
		if (GM)
		{
			UPP_QuestEventBridge* Bridge = GM->GetQuestEventBridge();
			if (Bridge)
			{
				Bridge->NotifyEnemyKilled(KillerPC, EnemyID);
				UE_LOG(LogPP, Log, TEXT("[Enemy] HandleDeath: NotifyEnemyKilled Killer=%s EnemyID=%s"),
					*KillerPC->GetName(), *EnemyID.ToString());
			}
		}
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[Enemy] HandleDeath: QUEST KILL NOT TRACKED - KillerPC=%s EnemyID=%s"),
			KillerPC ? *KillerPC->GetName() : TEXT("null"), *EnemyID.ToString());
	}

	// 金币掉落（可选）
	if (GoldDropAmount <= 0 || !KillerPC)
	{
		return;
	}

	APP_PlayerState* KillerPS = KillerPC->GetPlayerState<APP_PlayerState>();
	if (!IsValid(KillerPS))
	{
		return;
	}

	UPP_InventoryComponent* KillerInventory = KillerPS->GetInventoryComponent();
	if (!IsValid(KillerInventory))
	{
		return;
	}

	KillerInventory->AddGold(GoldDropAmount);
	UE_LOG(LogPP, Log, TEXT("[Enemy] HandleDeath: 向击杀者 %s 发放金币 %d"),
		*KillerPC->GetName(), GoldDropAmount);
}
