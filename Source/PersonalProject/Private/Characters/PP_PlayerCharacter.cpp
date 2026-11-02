// Chen Tianbao uses GAS to create online battle games

#include "Characters/PP_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "AlsAnimationInstance.h"
#include "AlsCameraComponent.h"
#include "AlsCameraSettings.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_Log.h"
#include "Framework/PP_SessionManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/PPTags.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Net/UnrealNetwork.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsGameplayTags.h"
#include "Utility/AlsVector.h"

APP_PlayerCharacter::APP_PlayerCharacter()
{
	// ALS 自管 Tick、移动、旋转，不覆盖 BaseCharacter/AAlsCharacter 的任何相关配置

	// 玩家依赖 ALS 相机与视角刷新，必须保证 AnimationInstance 是 UAlsAnimationInstance，
	// 否则 AAlsCharacter::Tick 会因守卫提前 return，相机不再刷新。
	bForceAlsAnimationBlueprint = true;

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	Tags.Add(PersonalTags::Player);

	// ===== ALS 相机组件（移自 AAlsCharacterExample） =====
	Camera = CreateDefaultSubobject<UAlsCameraComponent>(FName{TEXTVIEW("Camera")});
	Camera->SetupAttachment(GetMesh());
	Camera->SetRelativeRotation_Direct({0.0f, 90.0f, 0.0f});

	// ===== ALS 输入资产兜底（改父类后蓝图配置会被清空） =====
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

	// ===== ALS Character 自身 Settings/MovementSettings 兜底已在 PP_BaseCharacter 构造函数里完成 =====

	// ===== 相机组件资产兜底（改父类后 B_Als_Character 的相机配置会丢失） =====
	if (Camera)
	{
		static ConstructorHelpers::FObjectFinder<UAlsCameraSettings> DefaultCameraSettings(
			TEXT("/ALS/ALSCamera/Data/CS_Als_Default.CS_Als_Default"));
		if (DefaultCameraSettings.Succeeded())
		{
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

// ===== GAS 接入（保留原样，ASC 已在 PlayerState） =====

UAbilitySystemComponent* APP_PlayerCharacter::GetAbilitySystemComponent() const
{
	APP_PlayerState* PPplayerState = Cast<APP_PlayerState>(GetPlayerState());
	if (!IsValid(PPplayerState)) return nullptr;

	return PPplayerState->GetAbilitySystemComponent();
}

UAttributeSet* APP_PlayerCharacter::GetAttributeSet() const
{
	APP_PlayerState* PPplayerState = Cast<APP_PlayerState>(GetPlayerState());
	if (!IsValid(PPplayerState)) return nullptr;

	return PPplayerState->GetAttributeSet();
}

void APP_PlayerCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 角色外观恢复（不依赖 ASC，必须在 ASC 检查之前执行）
	RestoreCharacterSelection();

	if (!IsValid(GetAbilitySystemComponent()) || !HasAuthority()) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());
	GiveStartupAbilities();
	InitializeAttributes();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);

	ApplyStaminaPassiveEffects();
}

void APP_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 客户端角色外观恢复（ASC 检查之前执行）
	RestoreCharacterSelection();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);
}

void APP_PlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	UpdateStaminaStateTags();
	UpdateRollStaminaCost();
}

void APP_PlayerCharacter::UpdateStaminaStateTags()
{
	// 耐力消耗由服务端权威 GE 执行，状态 tag 只在服务端维护
	if (!HasAuthority() || !IsValid(GetAbilitySystemComponent()))
	{
		return;
	}

	// "真的在冲刺"才算：ALS 的 Gait 在速度为 0 时仍可能保持 Sprinting，
	// 必须叠加"在地面 + 无翻滚/倒地等动作"判断，避免站着不动或翻滚时掉耐力。
	const UCharacterMovementComponent* Movement = GetCharacterMovement();
	const bool bNowSprinting =
		IsAlive()
		&& GetGait() == AlsGaitTags::Sprinting
		&& !GetLocomotionAction().IsValid()
		&& Movement && Movement->IsMovingOnGround();

	const bool bHasSprintingTag = GetAbilitySystemComponent()->HasMatchingGameplayTag(PPTags::Status::Sprinting);
	if (bNowSprinting && !bHasSprintingTag)
	{
		GetAbilitySystemComponent()->AddLooseGameplayTag(PPTags::Status::Sprinting);
	}
	else if (!bNowSprinting && bHasSprintingTag)
	{
		GetAbilitySystemComponent()->RemoveLooseGameplayTag(PPTags::Status::Sprinting);
	}
}

void APP_PlayerCharacter::ApplyStaminaPassiveEffects()
{
	if (!IsValid(GetAbilitySystemComponent()))
	{
		return;
	}

	// 两个常驻 Infinite 周期 GE：是否真正生效由 OngoingTagRequirements 决定
	const TSubclassOf<UGameplayEffect> PassiveEffects[] = {StaminaRegenEffect, StaminaSprintDrainEffect};
	for (const TSubclassOf<UGameplayEffect>& EffectClass : PassiveEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
		ContextHandle.AddSourceObject(this);
		FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void APP_PlayerCharacter::UpdateRollStaminaCost()
{
	// 翻滚是 ALS 动作状态，只在服务端扣体力
	if (!HasAuthority() || !IsValid(GetAbilitySystemComponent()))
	{
		bWasRolling = false;
		return;
	}

	const bool bRolling = GetLocomotionAction() == AlsLocomotionActionTags::Rolling;
	if (bRolling && !bWasRolling)
	{
		ApplyStaminaCost(RollStaminaCost);
	}

	bWasRolling = bRolling;
}

void APP_PlayerCharacter::ApplyStaminaCost(float Amount)
{
	if (Amount <= 0.f || !StaminaCostEffect || !IsValid(GetAbilitySystemComponent()))
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(StaminaCostEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// SetByCaller 传负值，Instant GE 用加法扣减
	SpecHandle.Data->SetSetByCallerMagnitude(PPTags::SetByCaller::StaminaCost, -Amount);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void APP_PlayerCharacter::OnStaminaChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	// 只处理"消耗"（耐力下降）；回复不触发延迟
	if (AttributeChangeData.NewValue >= AttributeChangeData.OldValue || !IsAlive())
	{
		return;
	}

	if (!IsValid(GetAbilitySystemComponent()) || !StaminaRegenDelayEffect)
	{
		return;
	}

	// 每次消耗都刷新延迟窗口（GE 使用 Aggregate + Refresh 叠加策略，不会叠加堆栈）
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(StaminaRegenDelayEffect, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void APP_PlayerCharacter::ServerRespawn()
{
	if (IsAlive()) return;

	// 移除授予 Status.Dead 标签的 GE（原由死亡 GA 的 Respawn 流程移除，
	// 但服务端 GA 的蒙太奇回调会被 Ragdoll 卡死，改由这里兜底）
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));
	}

	Super::ServerRespawn();
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

	// 优先级3: SessionManager 的最后选择
	if (CharId.IsNone())
	{
		if (UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld()))
		{
			CharId = SM->GetLastCharacterSelection();
			if (!CharId.IsNone())
			{
				PS->SetSelectedCharacterId(CharId);
			}
		}
	}

	// 优先级4: DataManager 角色表第一行（项目默认角色，改数据表行顺序即可切换默认）
	if (CharId.IsNone())
	{
		if (UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld()))
		{
			CharId = DataManager->GetDefaultCharacterId();
			if (!CharId.IsNone())
			{
				PS->SetSelectedCharacterId(CharId);
				UE_LOG(LogPP, Log, TEXT("[Appearance] 未指定角色选择，回落到项目默认角色: %s"), *CharId.ToString());
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

	// 外观完全由 DT_Characters 决定：每个角色配自己的网格 + 该角色专属的 ALS 重定向动画蓝图。
	// 数据表留空时回落到 ALS 默认资产（曼尼昆 + AB_Als），保证未配置的角色仍可运行。
	USkeletalMesh* LoadedMesh = Row->CharacterMesh.LoadSynchronous();
	UClass* LoadedAnimClass = Row->AnimBlueprintClass.LoadSynchronous();

	if (!LoadedMesh)
	{
		static USkeletalMesh* CachedAlsMesh = LoadObject<USkeletalMesh>(
			nullptr, TEXT("/ALS/ALS/Character/SKM_Als.SKM_Als"));
		LoadedMesh = CachedAlsMesh;
	}

	if (!LoadedAnimClass)
	{
		static UClass* CachedAlsAnimClass = LoadClass<UAnimInstance>(
			nullptr, TEXT("/ALS/ALS/Character/AB_Als.AB_Als_C"));
		LoadedAnimClass = CachedAlsAnimClass;
	}

	if (!GetMesh() || (!LoadedMesh && !LoadedAnimClass)) return;

	if (LoadedMesh)
	{
		// 先断开上一套动画实例：PostInitializeComponents 会为 bForceAlsAnimationBlueprint 的角色
		// 强制套上曼尼昆的 AB_Als_C，这套实例连同它的一串联动子实例（AB_Als_Layering /
		// AB_Als_Standing / AB_Als_Grounded…）会一直残留在同一个网格组件上。
		// 它们虽然不是 AnimScriptInstance，但在特定视角/曲线权重下会短暂参与混合，
		// 把曼尼昆的骨骼长度（spine_01=10.84、neck_01=16.56）掺回躯干，表现为角色一瞬被拉长。
		GetMesh()->SetAnimInstanceClass(nullptr);

		GetMesh()->SetSkeletalMesh(LoadedMesh);
	}

	if (LoadedAnimClass)
	{
		GetMesh()->SetAnimInstanceClass(LoadedAnimClass);
	}

	// 换网格/动画类后动画实例会被重建，AAlsCharacter 缓存的 AnimationInstance 会失效
	// （失效会导致 ALS Tick 的守卫提前 return，相机与移动刷新全部停摆），这里重新缓存。
	GetMesh()->InitAnim(true);
	AnimationInstance = Cast<UAlsAnimationInstance>(GetMesh()->GetAnimInstance());

	if (!AnimationInstance.IsValid())
	{
		UE_LOG(LogPP, Error,
			TEXT("[Appearance] %s 的动画蓝图 %s 不是 UAlsAnimationInstance 子类，ALS 移动/相机会失效"),
			*GetNameSafe(this), *GetNameSafe(LoadedAnimClass));
	}
}

// ===== ALS 相机（移植自 AAlsCharacterExample） =====

namespace
{
	// 运行时兜底：蓝图父类变更后新 UPROPERTY 可能为空（未序列化），
	// 构造函数的 ConstructorHelpers 无法覆盖蓝图空值，只能在首次使用时重新 Load
	template<typename T>
	void EnsureAsset(TObjectPtr<T>& Ptr, const TCHAR* Path)
	{
		if (IsValid(Ptr)) return;
		Ptr = LoadObject<T>(nullptr, Path);
	}
}

void APP_PlayerCharacter::NotifyControllerChanged()
{
	// 运行时兜底：确保 ALS IMC 不为 null（蓝图空值会覆盖 C++ 构造函数）
	EnsureAsset(InputMappingContext, TEXT("/ALS/ALS/Data/Input/IMC_Als_Default.IMC_Als_Default"));

	const auto* PreviousPlayer{Cast<APlayerController>(PreviousController)};
	if (IsValid(PreviousPlayer))
	{
		auto* InputSubsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PreviousPlayer->GetLocalPlayer())};
		if (IsValid(InputSubsystem))
		{
			InputSubsystem->RemoveMappingContext(InputMappingContext);
		}
	}

	auto* NewPlayer{Cast<APlayerController>(GetController())};
	if (IsValid(NewPlayer))
	{
		NewPlayer->InputYawScale_DEPRECATED = 1.0f;
		NewPlayer->InputPitchScale_DEPRECATED = 1.0f;
		NewPlayer->InputRollScale_DEPRECATED = 1.0f;

		auto* InputSubsystem{ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(NewPlayer->GetLocalPlayer())};
		if (IsValid(InputSubsystem))
		{
			if (IsValid(InputMappingContext))
			{
				FModifyContextOptions Options;
				Options.bNotifyUserSettings = true;
				// 和 ALS 源码 AAlsCharacterExample::NotifyControllerChanged 保持一致（priority=0）
				InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
			}
			else
			{
				UE_LOG(LogPP, Error, TEXT("[ALS_Input] NotifyControllerChanged  InputMappingContext is STILL NULL after EnsureAsset!"));
			}
		}
		else
		{
			UE_LOG(LogPP, Error, TEXT("[ALS_Input] NotifyControllerChanged  InputSubsystem is NULL!"));
		}
	}

	Super::NotifyControllerChanged();
}

void APP_PlayerCharacter::CalcCamera(const float DeltaTime, FMinimalViewInfo& ViewInfo)
{
	if (Camera->IsActive())
	{
		Camera->GetViewInfo(ViewInfo);
		return;
	}

	Super::CalcCamera(DeltaTime, ViewInfo);
}

// ===== ALS 输入绑定（移植自 AAlsCharacterExample） =====

void APP_PlayerCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
	Super::SetupPlayerInputComponent(Input);

	// 运行时兜底：确保所有 IA 不为 null
	EnsureAsset(LookMouseAction, TEXT("/ALS/ALS/Data/Input/IA_Als_LookMouse.IA_Als_LookMouse"));
	EnsureAsset(LookAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Look.IA_Als_Look"));
	EnsureAsset(MoveAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Move.IA_Als_Move"));
	EnsureAsset(SprintAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Sprint.IA_Als_Sprint"));
	EnsureAsset(WalkAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Walk.IA_Als_Walk"));
	EnsureAsset(CrouchAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Crouch.IA_Als_Crouch"));
	EnsureAsset(JumpAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Jump.IA_Als_Jump"));
	EnsureAsset(AimAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Aim.IA_Als_Aim"));
	EnsureAsset(RagdollAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Ragdoll.IA_Als_Ragdoll"));
	EnsureAsset(RollAction, TEXT("/ALS/ALS/Data/Input/IA_Als_Roll.IA_Als_Roll"));
	EnsureAsset(RotationModeAction, TEXT("/ALS/ALS/Data/Input/IA_Als_RotationMode.IA_Als_RotationMode"));
	EnsureAsset(ViewModeAction, TEXT("/ALS/ALS/Data/Input/IA_Als_ViewMode.IA_Als_ViewMode"));
	EnsureAsset(SwitchShoulderAction, TEXT("/ALS/ALS/Data/Input/IA_Als_SwitchShoulder.IA_Als_SwitchShoulder"));

	auto* EnhancedInput{Cast<UEnhancedInputComponent>(Input)};
	if (IsValid(EnhancedInput))
	{
		EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLookMouse);
		EnhancedInput->BindAction(LookMouseAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnLookMouse);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnLook);
		EnhancedInput->BindAction(LookAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnLook);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnMove);
		EnhancedInput->BindAction(MoveAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnMove);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSprint);
		EnhancedInput->BindAction(SprintAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnSprint);
		EnhancedInput->BindAction(WalkAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnWalk);
		EnhancedInput->BindAction(CrouchAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnCrouch);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnJump);
		EnhancedInput->BindAction(JumpAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnJump);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnAim);
		EnhancedInput->BindAction(AimAction, ETriggerEvent::Canceled, this, &ThisClass::Input_OnAim);
		EnhancedInput->BindAction(RagdollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRagdoll);
		EnhancedInput->BindAction(RollAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRoll);
		EnhancedInput->BindAction(RotationModeAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnRotationMode);
		EnhancedInput->BindAction(ViewModeAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnViewMode);
		EnhancedInput->BindAction(SwitchShoulderAction, ETriggerEvent::Triggered, this, &ThisClass::Input_OnSwitchShoulder);
	}
	else
	{
		UE_LOG(LogPP, Error, TEXT("[ALS_Input] SetupPlayerInputComponent  EnhancedInputComponent is NULL!"));
	}
}

void APP_PlayerCharacter::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	const FVector2f Value{ActionValue.Get<FVector2D>()};

	AddControllerPitchInput(Value.Y * LookUpMouseSensitivity);
	AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}

void APP_PlayerCharacter::Input_OnLook(const FInputActionValue& ActionValue)
{
	const FVector2f Value{ActionValue.Get<FVector2D>()};

	AddControllerPitchInput(Value.Y * LookUpRate);
	AddControllerYawInput(Value.X * LookRightRate);
}

void APP_PlayerCharacter::Input_OnMove(const FInputActionValue& ActionValue)
{
	const auto Value{UAlsVector::ClampMagnitude012D(ActionValue.Get<FVector2D>())};

	auto ViewRotation{GetViewState().Rotation};

	if (IsValid(GetController()))
	{
		FVector ViewLocation;
		GetController()->GetPlayerViewPoint(ViewLocation, ViewRotation);
	}

	const auto ForwardDirection{UAlsVector::AngleToDirectionXY(UE_REAL_TO_FLOAT(ViewRotation.Yaw))};
	const auto RightDirection{UAlsVector::PerpendicularCounterClockwiseXY(ForwardDirection)};

	AddMovementInput(ForwardDirection * Value.Y + RightDirection * Value.X);
}

void APP_PlayerCharacter::Input_OnSprint(const FInputActionValue& ActionValue)
{
	SetDesiredGait(ActionValue.Get<bool>() ? AlsGaitTags::Sprinting : AlsGaitTags::Running);
}

void APP_PlayerCharacter::Input_OnWalk()
{
	if (GetDesiredGait() == AlsGaitTags::Walking)
	{
		SetDesiredGait(AlsGaitTags::Running);
	}
	else if (GetDesiredGait() == AlsGaitTags::Running)
	{
		SetDesiredGait(AlsGaitTags::Walking);
	}
}

void APP_PlayerCharacter::Input_OnCrouch()
{
	if (GetDesiredStance() == AlsStanceTags::Standing)
	{
		SetDesiredStance(AlsStanceTags::Crouching);
	}
	else if (GetDesiredStance() == AlsStanceTags::Crouching)
	{
		SetDesiredStance(AlsStanceTags::Standing);
	}
}

void APP_PlayerCharacter::Input_OnJump(const FInputActionValue& ActionValue)
{
	if (ActionValue.Get<bool>())
	{
		if (StopRagdolling())
		{
			return;
		}

		if (StartMantling())
		{
			return;
		}

		if (GetStance() == AlsStanceTags::Crouching)
		{
			SetDesiredStance(AlsStanceTags::Standing);
			return;
		}

		Jump();
	}
	else
	{
		StopJumping();
	}
}

void APP_PlayerCharacter::Input_OnAim(const FInputActionValue& ActionValue)
{
	SetDesiredAiming(ActionValue.Get<bool>());
}

void APP_PlayerCharacter::Input_OnRagdoll()
{
	if (!StopRagdolling())
	{
		StartRagdolling();
	}
}

void APP_PlayerCharacter::Input_OnRoll()
{
	static constexpr auto PlayRate{1.3f};

	// 体力不足以支付翻滚消耗时不翻滚（客户端用复制过来的体力值判断，服务端 Tick 里扣费）
	if (const UPP_AttributeSet* AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet()))
	{
		if (AttributeSet->GetStamina() < RollStaminaCost)
		{
			return;
		}
	}

	StartRollingGrounded(PlayRate);
}

void APP_PlayerCharacter::Input_OnRotationMode()
{
	SetDesiredRotationMode(GetDesiredRotationMode() == AlsRotationModeTags::VelocityDirection
			                       ? AlsRotationModeTags::ViewDirection
			                       : AlsRotationModeTags::VelocityDirection);
}

void APP_PlayerCharacter::Input_OnViewMode()
{
	SetViewMode(GetViewMode() == AlsViewModeTags::ThirdPerson ? AlsViewModeTags::FirstPerson : AlsViewModeTags::ThirdPerson);
}

// ReSharper disable once CppMemberFunctionMayBeConst
void APP_PlayerCharacter::Input_OnSwitchShoulder()
{
	Camera->SetRightShoulder(!Camera->IsRightShoulder());
}
