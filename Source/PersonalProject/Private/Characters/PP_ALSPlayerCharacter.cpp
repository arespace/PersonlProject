// ALS + GAS 集成 POC：APP_ALSPlayerCharacter 实现

#include "Characters/PP_ALSPlayerCharacter.h"

#include "AbilitySystem/PP_AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayAbilitySpec.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "AlsCameraComponent.h"
#include "AlsCameraSettings.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/PPTags.h"
#include "Utility/AlsGameplayTags.h"

APP_ALSPlayerCharacter::APP_ALSPlayerCharacter()
{
	// ALS 自身需要 Tick，不覆盖 PrimaryActorTick.bCanEverTick

	// ASC 直接挂角色上（POC 对齐 PP_EnemyCharacter 模式，Mixed 复制以支持客户端预测）
	AbilitySystemComponent = CreateDefaultSubobject<UPP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	AttributeSet = CreateDefaultSubobject<UPP_AttributeSet>("AttributeSet");

	// 专用服务器也需要刷新骨骼（与 PP_BaseCharacter 一致）
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 兜底加载 ALS 默认设置资产，避免改父类后丢失 B_Als_Character 的配置导致 Ensure 失败
	static ConstructorHelpers::FObjectFinder<UAlsCharacterSettings> DefaultCharacterSettings(
		TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default"));
	if (DefaultCharacterSettings.Succeeded())
	{
		Settings = DefaultCharacterSettings.Object;
	}

	static ConstructorHelpers::FObjectFinder<UAlsMovementSettings> DefaultMovementSettings(
		TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal"));
	if (DefaultMovementSettings.Succeeded())
	{
		MovementSettings = DefaultMovementSettings.Object;
	}

	// 兜底设置网格体与动画蓝图（改父类时 B_Als_Character 的组件配置会丢失）
	// 注意：ALS 中 SKM_Als 是 SkeletalMesh，SK_Als 是 Skeleton
	static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultMesh(
		TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
	if (DefaultMesh.Succeeded())
	{
		GetMesh()->SetSkeletalMesh(DefaultMesh.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> DefaultAnimClass(
		TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
	if (DefaultAnimClass.Succeeded())
	{
		GetMesh()->SetAnimInstanceClass(DefaultAnimClass.Class);
	}

	// 兜底加载 ALS 输入资产（改父类时 B_Als_Character 的输入配置会丢失）
	// 这些属性由 AAlsCharacterExample 定义，SetupPlayerInputComponent 中会绑定
	static ConstructorHelpers::FObjectFinder<UInputMappingContext> DefaultInputMappingContext(
		TEXT("/ALS/ALS/Data/Input/IMC_Als_Default.IMC_Als_Default"));
	if (DefaultInputMappingContext.Succeeded())
	{
		InputMappingContext = DefaultInputMappingContext.Object;
	}

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultLookMouseAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_LookMouse.IA_Als_LookMouse"));
	if (DefaultLookMouseAction.Succeeded()) { LookMouseAction = DefaultLookMouseAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultLookAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Look.IA_Als_Look"));
	if (DefaultLookAction.Succeeded()) { LookAction = DefaultLookAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultMoveAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Move.IA_Als_Move"));
	if (DefaultMoveAction.Succeeded()) { MoveAction = DefaultMoveAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultSprintAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Sprint.IA_Als_Sprint"));
	if (DefaultSprintAction.Succeeded()) { SprintAction = DefaultSprintAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultWalkAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Walk.IA_Als_Walk"));
	if (DefaultWalkAction.Succeeded()) { WalkAction = DefaultWalkAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultCrouchAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Crouch.IA_Als_Crouch"));
	if (DefaultCrouchAction.Succeeded()) { CrouchAction = DefaultCrouchAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultJumpAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Jump.IA_Als_Jump"));
	if (DefaultJumpAction.Succeeded()) { JumpAction = DefaultJumpAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultAimAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Aim.IA_Als_Aim"));
	if (DefaultAimAction.Succeeded()) { AimAction = DefaultAimAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultRagdollAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Ragdoll.IA_Als_Ragdoll"));
	if (DefaultRagdollAction.Succeeded()) { RagdollAction = DefaultRagdollAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultRollAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_Roll.IA_Als_Roll"));
	if (DefaultRollAction.Succeeded()) { RollAction = DefaultRollAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultRotationModeAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_RotationMode.IA_Als_RotationMode"));
	if (DefaultRotationModeAction.Succeeded()) { RotationModeAction = DefaultRotationModeAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultViewModeAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_ViewMode.IA_Als_ViewMode"));
	if (DefaultViewModeAction.Succeeded()) { ViewModeAction = DefaultViewModeAction.Object; }

	static ConstructorHelpers::FObjectFinder<UInputAction> DefaultSwitchShoulderAction(
		TEXT("/ALS/ALS/Data/Input/IA_Als_SwitchShoulder.IA_Als_SwitchShoulder"));
	if (DefaultSwitchShoulderAction.Succeeded()) { SwitchShoulderAction = DefaultSwitchShoulderAction.Object; }

	// 兜底配置 ALS 相机组件（改父类时 B_Als_Character 的相机配置会丢失）
	// Camera 由 AAlsCharacterExample 构造函数创建，此处补全其 Settings/Mesh/AnimClass
	if (Camera)
	{
		static ConstructorHelpers::FObjectFinder<UAlsCameraSettings> DefaultCameraSettings(
			TEXT("/ALS/ALSCamera/Data/CS_Als_Default.CS_Als_Default"));
		if (DefaultCameraSettings.Succeeded())
		{
			// Settings 是 protected 成员，通过反射设置
			if (FObjectPropertyBase* SettingsProp = FindFProperty<FObjectPropertyBase>(
				Camera->GetClass(), TEXT("Settings")))
			{
				SettingsProp->SetObjectPropertyValue_InContainer(Camera, DefaultCameraSettings.Object);
			}
		}

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> DefaultCameraMesh(
			TEXT("/ALS/ALSCamera/SKM_Als_Camera.SKM_Als_Camera"));
		if (DefaultCameraMesh.Succeeded())
		{
			Camera->SetSkeletalMesh(DefaultCameraMesh.Object);
		}

		static ConstructorHelpers::FClassFinder<UAnimInstance> DefaultCameraAnimClass(
			TEXT("/ALS/ALSCamera/AB_Als_Camera.AB_Als_Camera_C"));
		if (DefaultCameraAnimClass.Succeeded())
		{
			Camera->SetAnimInstanceClass(DefaultCameraAnimClass.Class);
		}
	}
}

void APP_ALSPlayerCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ThisClass, bAlive);
}

UAbilitySystemComponent* APP_ALSPlayerCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APP_ALSPlayerCharacter::PostInitializeComponents()
{
	// 兜底网格体与动画蓝图：改父类后蓝图可能保留错误的覆盖值（空或非 ALS 动画类），
	// 优先级高于 C++ 构造函数默认值。必须在 Super::PostInitializeComponents 之前设置好，
	// 因为 AAlsCharacter 会在那里执行 Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance())。
	// 注意：PostInitializeComponents 不是构造函数，不能用 ConstructorHelpers::FObjectFinder，
	// 必须用 LoadObject/LoadClass。

	if (!GetMesh()->GetSkeletalMeshAsset())
	{
		// ALS 中 SKM_Als 是 SkeletalMesh，SK_Als 是 Skeleton
		static USkeletalMesh* CachedDefaultMesh = LoadObject<USkeletalMesh>(
			nullptr, TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
		if (CachedDefaultMesh)
		{
			GetMesh()->SetSkeletalMesh(CachedDefaultMesh);
		}
	}

	// POC 强制使用 ALS 动画蓝图 AB_Als（无论蓝图当前配的是什么）
	static UClass* CachedDefaultAnimClass = LoadClass<UAnimInstance>(
		nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
	if (CachedDefaultAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(CachedDefaultAnimClass);
	}

	// AnimClass 可能在 InitializeComponent 阶段为空（蓝图空覆盖），此处需重新初始化动画实例
	if (GetMesh()->GetAnimClass() && !GetMesh()->GetAnimInstance())
	{
		GetMesh()->InitAnim(true);
	}

	Super::PostInitializeComponents();
}

void APP_ALSPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	if (!IsValid(GetAbilitySystemComponent())) return;

	// 服务端与客户端都需要 InitAbilityActorInfo（客户端用于属性复制回调绑定）
	GetAbilitySystemComponent()->InitAbilityActorInfo(this, this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	// 技能授予与属性初始化仅在服务端执行
	if (!HasAuthority()) return;

	GiveStartupAbilities();
	InitializeAttributes();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(
		PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
}

void APP_ALSPlayerCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;

	for (const auto& Ability : StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void APP_ALSPlayerCharacter::InitializeAttributes() const
{
	checkf(IsValid(InitializeAttributesEffect), TEXT("InitializeAttributesEffect not set."));

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(
		InitializeAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void APP_ALSPlayerCharacter::OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	if (AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
}

void APP_ALSPlayerCharacter::HandleDeath()
{
	bAlive = false;
}

void APP_ALSPlayerCharacter::HandleRespawn()
{
	bAlive = true;
}

void APP_ALSPlayerCharacter::ResetAttributes()
{
	checkf(IsValid(ResetAttributesEffect), TEXT("ResetAttributesEffect not set."));
	if (!IsValid(GetAbilitySystemComponent())) return;

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(
		ResetAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
