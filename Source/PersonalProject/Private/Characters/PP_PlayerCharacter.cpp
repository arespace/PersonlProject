// Chen Tianbao uses GAS to create online battle games


#include "Characters/PP_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Components/CapsuleComponent.h"
#include "Framework/PP_DataManager.h"
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
	RestoreCharacterSelection();

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
	RestoreCharacterSelection();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(),this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(),GetAttributeSet());

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void APP_PlayerCharacter::RestoreCharacterSelection()
{
	APP_PlayerState* PS = GetPlayerState<APP_PlayerState>();
	if (!PS) return;

	// 优先级1: PlayerState 已复制的值（最权威）
	FName CharId = PS->GetSelectedCharacterId();

	// 优先级2: PlayerController 的 PendingCharacterId（跨 ServerTravel 传递）
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

	// 优先级3: GameInstance（转发到 SessionManager）的最后选择
	if (CharId.IsNone())
	{
		if (UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld()))
		{
			CharId = GI->GetLastCharacterSelection();
			if (!CharId.IsNone())
			{
				PS->SetSelectedCharacterId(CharId);
			}
		}
	}

	ApplyCharacterAppearance(CharId);
}

void APP_PlayerCharacter::ApplyCharacterAppearance(FName CharacterId)
{
	if (CharacterId.IsNone()) return;

	// 统一走 DataManager 全局角色表
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	if (!DataManager) return;

	const FCharacterDataRow* Row = DataManager->GetCharacterData(CharacterId);
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


