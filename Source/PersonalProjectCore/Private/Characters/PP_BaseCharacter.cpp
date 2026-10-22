// Chen Tianbao uses GAS to create online battle games


#include "Characters/PP_BaseCharacter.h"

#include "Animation/AnimInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"
#include "Settings/AlsCharacterSettings.h"
#include "Settings/AlsMovementSettings.h"
#include "UObject/ConstructorHelpers.h"

namespace PersonalTags
{
	const FName Player = FName("Player");
}

APP_BaseCharacter::APP_BaseCharacter()
{
	// ALS 需要 Tick 驱动视图/移动刷新，不得禁用 PrimaryActorTick

	//Tick and refresh bone transforms whether rendered or not - for bone updates on a dedicated server
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;

	// 兜底加载 ALS 默认设置资产：改父类后蓝图配置会被清空，缺失会触发 ALS_ENSURE 且移动不刷新
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
}

void APP_BaseCharacter::PostInitializeComponents()
{
	// ===== ALS Settings/MovementSettings 最终兜底 =====
	// 蓝图属性在 C++ 构造函数之后应用——如果 BP_PP_PlayerCharacter 里 Settings 设成 null
	// 会覆盖掉构造函数的兜底。PostInitializeComponents 时机最晚，这里再补一次。
	if (!Settings)
	{
		static UAlsCharacterSettings* CachedDefaultCharSettings = LoadObject<UAlsCharacterSettings>(
			nullptr, TEXT("/ALS/ALS/Data/Character/CS_Als_Default.CS_Als_Default"));
		Settings = CachedDefaultCharSettings;
		UE_LOG(LogTemp, Warning, TEXT("[ALS_FIX] Settings was NULL in PostInitializeComponents — applied fallback CS_Als_Default"));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[ALS_FIX] Settings OK: %s"), *GetNameSafe(Settings));
	}

	if (!MovementSettings)
	{
		static UAlsMovementSettings* CachedDefaultMoveSettings = LoadObject<UAlsMovementSettings>(
			nullptr, TEXT("/ALS/ALS/Data/Character/Movement/MS_Als_Normal.MS_Als_Normal"));
		MovementSettings = CachedDefaultMoveSettings;
		UE_LOG(LogTemp, Warning, TEXT("[ALS_FIX] MovementSettings was NULL — applied fallback MS_Als_Normal"));
	}

	// ===== 网格体与动画蓝图兜底 =====
	// 必须在 Super::PostInitializeComponents 之前设置好，因为 AAlsCharacter 会在那里
	// 执行 Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance()) 来设置 AnimationInstance。
	// 如果蓝图里配了非 ALS 的 Anim BP（如 Greystone_AnimBlueprint），AnimationInstance 就是 null，
	// ALS Tick 守卫 !AnimationInstance.IsValid() 会直接 return，RefreshView 永远不跑。

	if (!GetMesh()->GetSkeletalMeshAsset())
	{
		static USkeletalMesh* CachedDefaultMesh = LoadObject<USkeletalMesh>(
			nullptr, TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
		if (CachedDefaultMesh)
		{
			GetMesh()->SetSkeletalMesh(CachedDefaultMesh);
		}
	}

	// 强制用 ALS 动画蓝图——之前是 `if (!GetAnimClass())`，但蓝图可能配了非 ALS 的 Anim BP
	// （如 Greystone），那样 AnimationInstance 永远不会被 AAlsCharacter::PostInitializeComponents 初始化
	static UClass* CachedDefaultAnimClass = LoadClass<UAnimInstance>(
		nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
	if (CachedDefaultAnimClass && GetMesh()->GetAnimClass() != CachedDefaultAnimClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ALS_FIX] Overriding AnimClass %s → AB_Als"), 
			*GetNameSafe(GetMesh()->GetAnimClass()));
		GetMesh()->SetAnimInstanceClass(CachedDefaultAnimClass);
	}

	Super::PostInitializeComponents();
}

void APP_BaseCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bAlive);
}

UAbilitySystemComponent* APP_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}


void APP_BaseCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;
	for (const auto& Ability	: StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void APP_BaseCharacter::InitializeAttributes() const
{
	checkf(IsValid(InitializeAttributesEffect) , TEXT("InitializeAttributesEffect not set."));
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void APP_BaseCharacter::OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	if (AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
}

void APP_BaseCharacter::HandleDeath()
{
	bAlive = false;
}

void APP_BaseCharacter::HandleRespawn()
{
	bAlive = true;
}

void APP_BaseCharacter::ResetAttributes()
{
	checkf(IsValid(ResetAttributesEffect) , TEXT("ResetAttributesEffect not set."));
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(ResetAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

