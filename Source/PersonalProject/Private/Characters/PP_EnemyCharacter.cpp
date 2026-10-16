// Chen Tianbao uses GAS to create online battle games


#include "Characters/PP_EnemyCharacter.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/PP_AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "GameplayTags/PPTags.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Player/PP_PlayerState.h"
#include "Runtime/AIModule/Classes/AIController.h"
#include "Net/UnrealNetwork.h"
#include "Quest/PP_QuestEventBridge.h"
#include "Game/PP_CrashGameMode.h"
#include "Framework/PP_Log.h"


APP_EnemyCharacter::APP_EnemyCharacter()
{
	PrimaryActorTick.bCanEverTick = false;

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

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(this,this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());

	if (!HasAuthority()) return;

	GiveStartupAbilities();
	InitializeAttributes();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
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
