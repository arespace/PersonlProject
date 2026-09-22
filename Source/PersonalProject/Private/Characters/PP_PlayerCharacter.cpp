// Chen Tianbao uses GAS to create online battle games


#include "PersonalProject/Public/Characters/PP_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Engine/DataTable.h"
#include "Game/PP_GameInstance.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"


APP_PlayerCharacter::APP_PlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	GetCapsuleComponent()->InitCapsuleSize(42.f,96.f);
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;
	bUseControllerRotationYaw = false;
	
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.0f,540.0f,0.0f);
	GetCharacterMovement()->JumpZVelocity = 500.f;
	GetCharacterMovement()->AirControl = 0.35f;
	GetCharacterMovement()->MaxWalkSpeed = 500.f;
	GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
	GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
	GetCharacterMovement()->BrakingDecelerationFalling = 1500.f;
	
	
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>("CameraBoom");
	CameraBoom->SetupAttachment(GetRootComponent());
	CameraBoom->TargetArmLength = 300.0f;
	CameraBoom->bUsePawnControlRotation = true;
	
	FollowCamera = CreateDefaultSubobject<UCameraComponent>("FollowCamera");
	FollowCamera->SetupAttachment(CameraBoom,USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	Tags.Add(PersonalTags::Player);
}

UAbilitySystemComponent* APP_PlayerCharacter::GetAbilitySystemComponent() const
{
	APP_PlayerState* PPplayerState =  Cast<APP_PlayerState>(GetPlayerState());
	if (!IsValid(PPplayerState)) return nullptr;
	
	return PPplayerState->GetAbilitySystemComponent();
}

UAttributeSet* APP_PlayerCharacter::GetAttributeSet() const
{
	APP_PlayerState* PPplayerState =  Cast<APP_PlayerState>(GetPlayerState());
	if (!IsValid(PPplayerState)) return nullptr;
	
	return PPplayerState->GetAttributeSet();
}

void APP_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 角色外观恢复（不依赖 ASC，必须在 ASC 检查之前执行）
	if (APP_PlayerState* PS = GetPlayerState<APP_PlayerState>())
	{
		FName CharId = PS->GetSelectedCharacterId();

		// 优先级1: 从 PlayerController 读取（最可靠，PC 跨地图不销毁）
		if (CharId.IsNone())
		{
			if (APP_PlayerController* PC = Cast<APP_PlayerController>(GetController()))
			{
				CharId = PC->GetPendingCharacterId();


				if (!CharId.IsNone())
				{
					PS->SetSelectedCharacterId(CharId);
					UE_LOG(LogTemp, Log, TEXT("[PossessedBy] Restored from PlayerController: %s"), *CharId.ToString());
				}
			}
		}

		// 优先级2: 从 GameInstance 读取（PlayerName 匹配可能失败）
		if (CharId.IsNone())
		{
			if (UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld()))
			{
				CharId = GI->GetPlayerCharacterSelection(PS->GetPlayerName());
				if (!CharId.IsNone())
				{
					PS->SetSelectedCharacterId(CharId);
					UE_LOG(LogTemp, Log, TEXT("[PossessedBy] Restored from GameInstance: %s"), *CharId.ToString());
				}
			}
		}

		ApplyCharacterAppearance(CharId);
	}

	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(),this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());
	GiveStartupAbilities();
	InitializeAttributes();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void APP_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 客户端角色外观恢复（ASC 检查之前执行）
	if (APP_PlayerState* PS = GetPlayerState<APP_PlayerState>())
	{
		FName CharId = PS->GetSelectedCharacterId();

		if (CharId.IsNone())
		{
			if (APP_PlayerController* PC = Cast<APP_PlayerController>(GetController()))
			{
				CharId = PC->GetPendingCharacterId();
				if (!CharId.IsNone())
				{
					PS->SetSelectedCharacterId(CharId);
				}
			}
		}

		if (CharId.IsNone())
		{
			if (UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld()))
			{
				CharId = GI->GetPlayerCharacterSelection(PS->GetPlayerName());
				if (!CharId.IsNone())
				{
					PS->SetSelectedCharacterId(CharId);
				}
			}
		}

		ApplyCharacterAppearance(CharId);
	}

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(),this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void APP_PlayerCharacter::ApplyCharacterAppearance(FName CharacterId)
{
	if (CharacterId.IsNone() || !CharacterDataTable) return;

	// 从 DataTable 查找角色数据
	static const FString ContextString(TEXT("CharacterAppearance"));
	FCharacterDataRow* Row = CharacterDataTable->FindRow<FCharacterDataRow>(CharacterId, ContextString);
	if (!Row) return;

	// 应用骨骼网格体
	USkeletalMesh* LoadedMesh = Row->CharacterMesh.LoadSynchronous();
	if (LoadedMesh && GetMesh())
	{
		GetMesh()->SetSkeletalMesh(LoadedMesh);
	}

	// 应用动画蓝图
	UClass* AnimClass = Row->AnimBlueprintClass.LoadSynchronous();
	if (AnimClass && GetMesh())
	{
		GetMesh()->SetAnimInstanceClass(AnimClass);
	}
}


