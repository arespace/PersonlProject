
#include "Characters/PP_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "AlsAnimationInstance.h"
#include "AlsCameraComponent.h"
#include "AlsCameraSettings.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/PP_EnemyCharacter.h"
#include "Characters/PP_StaminaComponent.h"
#include "Characters/PP_WeaponAbilityComponent.h"
#include "Characters/PP_WeaponVisualComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PP_ItemTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_Log.h"
#include "Session/PP_SessionManager.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayTags/PPTags.h"
#include "GameObjects/PP_SoulPickupActor.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Utility/AlsConstants.h"
#include "Utility/AlsGameplayTags.h"
#include "Utility/AlsVector.h"

APP_PlayerCharacter::APP_PlayerCharacter()
{
	// ALS 自管 Tick、移动、旋转，不覆盖 BaseCharacter/AAlsCharacter 的任何相关配置

	// 玩家依赖 ALS 相机与视角刷新，必须保证 AnimationInstance 是 UAlsAnimationInstance，
	// 否则 AAlsCharacter::Tick 会因守卫提前 return，相机不再刷新。
	bForceAlsAnimationBlueprint = true;

	// 默认掉落魂用 C++ 占位类，蓝图里可替换成自定义 BP
	SoulPickupClass = APP_SoulPickupActor::StaticClass();

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	Tags.Add(PPActorTags::Player);

	// ===== 武器外观组件（装备武器网格 + 自带烘焙武器隐藏） =====
	WeaponVisualComponent = CreateDefaultSubobject<UPP_WeaponVisualComponent>(TEXT("WeaponVisualComponent"));

	// ===== 武器技能组件（装备武器 → 授予/移除 GA，仅服务器） =====
	WeaponAbilityComponent = CreateDefaultSubobject<UPP_WeaponAbilityComponent>(TEXT("WeaponAbilityComponent"));


	// ===== ALS 相机组件（移自 AAlsCharacterExample） =====
	Camera = CreateDefaultSubobject<UAlsCameraComponent>(FName{TEXTVIEW("Camera")});
	Camera->SetupAttachment(GetMesh());
	Camera->SetRelativeRotation_Direct({0.0f, 90.0f, 0.0f});

	// ===== 锁定组件 =====
	LockOnComponent = CreateDefaultSubobject<UPP_LockOnComponent>(TEXT("LockOnComponent"));

	// ===== 耐力组件 =====
	StaminaComponent = CreateDefaultSubobject<UPP_StaminaComponent>(TEXT("StaminaComponent"));

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

// ===== 武器外观（实现已迁移到 UPP_WeaponVisualComponent，这里只保留薄外壳） =====

void APP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 武器外观（挂 socket / 识别材质槽 / 隐藏自带武器）已由 WeaponVisualComponent::BeginPlay 负责
}

void APP_PlayerCharacter::SetBuiltInWeaponVisible(bool bVisible)
{
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetBuiltInWeaponVisible(bVisible);
	}
}

void APP_PlayerCharacter::SetWeaponMesh(FName ItemID)
{
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->SetWeaponMesh(ItemID);
	}
}

void APP_PlayerCharacter::ClearWeaponMesh()
{
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->ClearWeaponMesh();
	}
}

// ===== 调试命令 =====

void APP_PlayerCharacter::PPAddItem(FName ItemID, int32 Count)
{
	if (HasAuthority())
	{
		GrantItemToInventory(ItemID, Count);
		return;
	}

	// 客户端窗口执行时转发到服务器
	ServerPPAddItem(ItemID, Count);
}

void APP_PlayerCharacter::ServerPPAddItem_Implementation(FName ItemID, int32 Count)
{
	GrantItemToInventory(ItemID, Count);
}

void APP_PlayerCharacter::GrantItemToInventory(FName ItemID, int32 Count)
{
	if (!HasAuthority())
	{
		UE_LOG(LogPP, Warning, TEXT("[Debug] PPAddItem: 只能在服务器执行"));
		return;
	}

	const APP_PlayerState* PPPlayerState = Cast<APP_PlayerState>(GetPlayerState());
	UPP_InventoryComponent* Inventory = PPPlayerState ? PPPlayerState->GetInventoryComponent() : nullptr;
	if (!IsValid(Inventory))
	{
		UE_LOG(LogPP, Warning, TEXT("[Debug] PPAddItem: PlayerState 上未找到背包组件"));
		return;
	}

	const int32 FinalCount = FMath::Max(1, Count);
	if (Inventory->AddItem(ItemID, FinalCount))
	{
		UE_LOG(LogPP, Log, TEXT("[Debug] PPAddItem: 已添加 %s x%d"), *ItemID.ToString(), FinalCount);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[Debug] PPAddItem: 添加 %s 失败，检查 DT_Items 是否存在该行且为装备类型"), *ItemID.ToString());
	}
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

	// 恢复 PlayerState 上已复制的武器状态（复活 / 重新进入关卡）：外观由组件按装备恢复，服务端补授予 GA
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->RefreshVisualFromInventory();
	}
	RestoreEquippedWeaponAbility();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	// 先 RemoveAll 再 Add：监听服务器上 PossessedBy 与 OnRep_PlayerState 都会执行，
	// 直接 AddUObject 会重复绑定，导致一次掉血触发两次 OnHealthChanged → HandleDeath 执行两次
	// （日志里出现 "bAlive=1" 和 "bAlive=0" 两条服务端死亡记录）。
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).RemoveAll(this);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);

	// 耐力委托与常驻 GE 由组件负责（组件内部同样先 RemoveAll 再 Add）
	if (StaminaComponent)
	{
		StaminaComponent->BindToAbilitySystem();
		StaminaComponent->ApplyPassiveEffects();
	}
}

void APP_PlayerCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 客户端角色外观恢复（ASC 检查之前执行）
	RestoreCharacterSelection();

	if (!IsValid(GetAbilitySystemComponent())) return;

	GetAbilitySystemComponent()->InitAbilityActorInfo(GetPlayerState(), this);
	OnASCInitialized.Broadcast(GetAbilitySystemComponent(), GetAttributeSet());

	// 客户端按已复制的武器槽恢复外观（GA 由服务端授予并复制）
	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->RefreshVisualFromInventory();
	}
	RestoreEquippedWeaponAbility();

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	// 同上：避免 PossessedBy / OnRep_PlayerState 重复绑定
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).RemoveAll(this);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);

	if (StaminaComponent)
	{
		StaminaComponent->BindToAbilitySystem();
	}
}

void APP_PlayerCharacter::Tick(float DeltaTime)
{
	// 排错：记录本机玩家的长帧（>50ms），用于区分"掉帧"与"输入/状态异常"。
	// 每秒最多打一条，避免刷屏。
	if (IsLocallyControlled() && DeltaTime > 0.05f)
	{
		static double LastHitchLogTime{0.0};
		const double NowTime{GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0};
		if (NowTime - LastHitchLogTime > 1.0)
		{
			LastHitchLogTime = NowTime;
			const APlayerController* PC{Cast<APlayerController>(GetController())};
			UE_LOG(LogPP, Warning,
				TEXT("[PP_Hitch] 长帧 %.1fms @%.2fs %s 存活=%d 移动模式=%d 速度=%.0f 输入忽略(移动=%d 视角=%d) LocomotionAction=%s"),
				DeltaTime * 1000.f, NowTime, *GetNameSafe(this), IsAlive() ? 1 : 0,
				GetCharacterMovement() ? static_cast<int32>(GetCharacterMovement()->MovementMode) : -1,
				GetVelocity().Size(),
				PC && PC->IsMoveInputIgnored() ? 1 : 0,
				PC && PC->IsLookInputIgnored() ? 1 : 0,
				*GetLocomotionAction().ToString());
		}
	}

	// 在 ALS Tick 之前更新相机朝向，保证同一帧的视角/角色旋转直接使用锁定方向
	if (LockOnComponent)
	{
		LockOnComponent->UpdateLockOn(DeltaTime);
	}

	Super::Tick(DeltaTime);

	// 耐力：冲刺 tag 维护 + 翻滚起始扣费（顺序与迁移前一致，仍在 i-frames 之前）
	if (StaminaComponent)
	{
		StaminaComponent->UpdateStamina();
	}

	UpdateRollInvulnerability();
}

void APP_PlayerCharacter::UpdateRollInvulnerability()
{
	// i-frames：跟随 ALS Rolling 状态给/移 Status.Invulnerable，
	// 伤害入口在 PP_BaseCharacter::CanBeDamaged 统一读取该标签。
	// 与组件的冲刺 tag 同一模式：loose tag 仅服务端维护。
	if (!HasAuthority() || !IsValid(GetAbilitySystemComponent()))
	{
		return;
	}

	const bool bRolling = IsAlive() && GetLocomotionAction() == AlsLocomotionActionTags::Rolling;
	const bool bHasInvulnerableTag = GetAbilitySystemComponent()->HasMatchingGameplayTag(PPTags::Status::Invulnerable);

	if (bRolling && !bHasInvulnerableTag)
	{
		GetAbilitySystemComponent()->AddLooseGameplayTag(PPTags::Status::Invulnerable);
	}
	else if (!bRolling && bHasInvulnerableTag)
	{
		GetAbilitySystemComponent()->RemoveLooseGameplayTag(PPTags::Status::Invulnerable);
	}
}

void APP_PlayerCharacter::ServerRespawn()
{
	if (IsAlive()) return;

	// 排错：记录计时器复活入口被调用时的输入状态
	const APlayerController* PC{Cast<APlayerController>(GetController())};
	UE_LOG(LogPP, Warning,
		TEXT("[PP_RespawnFlow] ServerRespawn 前: %s 本机=%d 控制者=%s 输入忽略(移动=%d 视角=%d)"),
		*GetNameSafe(this), IsLocallyControlled() ? 1 : 0, *GetNameSafe(PC),
		PC && PC->IsMoveInputIgnored() ? 1 : 0,
		PC && PC->IsLookInputIgnored() ? 1 : 0);

	// 移除授予 Status.Dead 标签的 GE（原由死亡 GA 的 Respawn 流程移除，
	// 但服务端 GA 的蒙太奇回调会被 Ragdoll 卡死，改由这里兜底）
	if (UAbilitySystemComponent* ASC = GetAbilitySystemComponent())
	{
		ASC->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead));
	}

	Super::ServerRespawn();
}

bool APP_PlayerCharacter::GetRespawnTransform(FTransform& OutTransform)
{
	// 优先使用玩家在篝火记录的重生点
	if (const APP_PlayerState* PS = GetPlayerState<APP_PlayerState>())
	{
		if (PS->HasCheckpoint())
		{
			OutTransform = PS->GetCheckpointTransform();
			return true;
		}
	}

	// 没有篝火记录时回退到 PlayerStart
	return Super::GetRespawnTransform(OutTransform);
}

void APP_PlayerCharacter::HandleDeath()
{
	// 基类对服务端重复进入只做忽略，不会阻断本重写函数的后续逻辑；
	// 这里记录进入前的存活状态，避免同一次死亡被重复计入死亡统计。
	const bool bWasAlive = IsAlive();

	// 基类负责：置死亡标记、Ragdoll、停移动、禁输入、启动复活计时
	Super::HandleDeath();

	if (!HasAuthority() || !bWasAlive || !GetWorld())
	{
		return;
	}

	// 死亡统计（服务端权威，随 PlayerState 复制到所有客户端）
	if (APP_PlayerState* PS = GetPlayerState<APP_PlayerState>())
	{
		PS->AddDeath();
	}

	// 上一次死亡留下且未拾回的魂：再次死亡时直接消散（魂类规则：二次死亡丢失）
	if (ActiveSoul.IsValid())
	{
		ActiveSoul->Destroy();
		ActiveSoul.Reset();
	}

	APP_PlayerState* PS = GetPlayerState<APP_PlayerState>();
	UPP_InventoryComponent* Inventory = PS ? PS->GetInventoryComponent() : nullptr;
	if (!IsValid(Inventory))
	{
		return;
	}

	const int32 CarriedGold = Inventory->GetGold();
	const int32 DropAmount = FMath::FloorToInt(
		static_cast<float>(CarriedGold) * FMath::Clamp(DeathSoulDropRatio, 0.f, 1.f));
	if (DropAmount <= 0)
	{
		return;
	}

	// 掉在角色脚下，避免魂的碰撞球与地面/胶囊重叠
	FVector DropLocation = GetActorLocation();
	if (const UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		DropLocation.Z -= FMath::Max(0.f, Capsule->GetScaledCapsuleHalfHeight() - 30.f);
	}

	UClass* SoulClass = SoulPickupClass ? SoulPickupClass.Get() : APP_SoulPickupActor::StaticClass();
	APP_SoulPickupActor* Soul = GetWorld()->SpawnActor<APP_SoulPickupActor>(
		SoulClass, DropLocation, FRotator::ZeroRotator);
	if (!IsValid(Soul))
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_SoulFlow] 死亡掉魂失败：SpawnActor 返回空 玩家=%s"), *GetNameSafe(PS));
		return;
	}

	// 先确认扣款成功，避免生成一个没有真正扣除金币的魂
	if (!Inventory->SpendGold(DropAmount))
	{
		Soul->Destroy();
		return;
	}

	Soul->InitializeSoul(PS, DropAmount);
	ActiveSoul = Soul;

	UE_LOG(LogPP, Warning, TEXT("[PP_SoulFlow] 死亡掉魂: 玩家=%s 金币 %d/%d 位置=(%.0f, %.0f, %.0f)"),
		*GetNameSafe(PS), DropAmount, CarriedGold, DropLocation.X, DropLocation.Y, DropLocation.Z);

	if (APP_PlayerController* PC = Cast<APP_PlayerController>(GetController()))
	{
		PC->Client_NotifyMessage(FString::Printf(TEXT("你丢失了 %d 金币，回到死亡地点可拾回"), DropAmount));
	}
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
				// 处于 Modal / FrontEnd 时不要把移动 IMC 加回来，否则重生会让角色重新能动
				if (bLocomotionInputEnabled)
				{
					FModifyContextOptions Options;
					Options.bNotifyUserSettings = true;
					// 和 ALS 源码 AAlsCharacterExample::NotifyControllerChanged 保持一致（priority=0）
					InputSubsystem->AddMappingContext(InputMappingContext, 0, Options);
				}
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

void APP_PlayerCharacter::SetLocomotionInputEnabled(bool bEnabled)
{
	if (bLocomotionInputEnabled == bEnabled)
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =
		PC ? ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()) : nullptr;
	if (!IsValid(InputSubsystem) || !IsValid(InputMappingContext))
	{
		return;
	}

	if (bEnabled)
	{
		InputSubsystem->AddMappingContext(InputMappingContext, 0);
	}
	else
	{
		InputSubsystem->RemoveMappingContext(InputMappingContext);
	}

	bLocomotionInputEnabled = bEnabled;
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

		// 锁定键走 IA：按键由 IMC_Abilities 提供（IA_LockOn 默认数字键 6）
		if (LockOnAction)
		{
			EnhancedInput->BindAction(LockOnAction, ETriggerEvent::Started, this, &ThisClass::Input_OnLockOn);
		}
		else
		{
			UE_LOG(LogPP, Warning, TEXT("[PP_Input] LockOnAction 未指定，锁定键不会生效。请在 BP_PlayerCharacter 的 PP|LockOn -> LockOnAction 挂上 IA_LockOn"));
		}

		// 装备长剑走 IA：原来由蓝图 EventGraph 的 F 键原生事件调 Server_EquipItem，现统一收到 C++
		if (EquipLongswordAction)
		{
			EnhancedInput->BindAction(EquipLongswordAction, ETriggerEvent::Started, this, &ThisClass::EquipLongsword);
		}
		else
		{
			UE_LOG(LogPP, Warning, TEXT("[PP_Input] EquipLongswordAction 未指定，装备长剑键不会生效。请在 BP_PlayerCharacter 的 PP|Input|Debug -> EquipLongswordAction 挂上 IA_EquipLongsword"));
		}
	}
	else
	{
		UE_LOG(LogPP, Error, TEXT("[ALS_Input] SetupPlayerInputComponent  EnhancedInputComponent is NULL!"));
	}
}

void APP_PlayerCharacter::Input_OnLookMouse(const FInputActionValue& ActionValue)
{
	// 锁定期间相机由目标驱动，忽略鼠标视角
	if (IsLockedOn())
	{
		return;
	}

	const FVector2f Value{ActionValue.Get<FVector2D>()};

	AddControllerPitchInput(Value.Y * LookUpMouseSensitivity);
	AddControllerYawInput(Value.X * LookRightMouseSensitivity);
}

void APP_PlayerCharacter::Input_OnLook(const FInputActionValue& ActionValue)
{
	// 锁定期间相机由目标驱动，忽略视角输入
	if (IsLockedOn())
	{
		return;
	}

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
	// 手动布娃娃同样走 ALS 的 IsRagdollingAllowedToStart()，物理资产缺少
	// pelvis / spine_03 时会在 Development 构建里触发 ensure 断点，这里直接忽略。
	if (GetMesh()->GetBodyInstance(UAlsConstants::PelvisBoneName()) == nullptr ||
	    GetMesh()->GetBodyInstance(UAlsConstants::Spine03BoneName()) == nullptr)
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Ragdoll] 忽略手动布娃娃：物理资产=%s 缺少 pelvis/spine_03"),
			*GetNameSafe(GetMesh()->GetPhysicsAsset()));
		return;
	}

	if (!StopRagdolling())
	{
		StartRagdolling();
	}
}

void APP_PlayerCharacter::Input_OnRoll()
{
	static constexpr auto PlayRate{1.3f};

	// 体力不足以支付翻滚消耗时不翻滚（客户端用复制过来的体力值判断，服务端 Tick 里扣费）
	if (StaminaComponent && !StaminaComponent->HasEnoughStamina(StaminaComponent->RollStaminaCost))
	{
		return;
	}

	StartRollingGrounded(PlayRate);
}

UAnimMontage* APP_PlayerCharacter::SelectRollMontage_Implementation()
{
	USkeletalMesh* MeshAsset = GetMesh() ? GetMesh()->GetSkeletalMeshAsset() : nullptr;
	const FString MeshName = MeshAsset ? MeshAsset->GetName() : TEXT("None");

	if (MeshAsset)
	{
		if (const TObjectPtr<UAnimMontage>* FoundMontage = RollMontageByMesh.Find(MeshAsset))
		{
			if (IsValid(*FoundMontage))
			{
				UE_LOG(LogPP, Log, TEXT("[Roll] 网格 %s 命中 RollMontageByMesh，使用 %s"),
					*MeshName, *FoundMontage->GetPathName());
				return *FoundMontage;
			}
		}
	}

	// 未配置该模型的翻滚动作，沿用 ALS 默认蒙太奇
	UAnimMontage* DefaultMontage = Super::SelectRollMontage_Implementation();
	UE_LOG(LogPP, Log, TEXT("[Roll] 网格 %s 未命中，回落 ALS 默认：%s"),
		*MeshName, DefaultMontage ? *DefaultMontage->GetPathName() : TEXT("None"));
	return DefaultMontage;
}

void APP_PlayerCharacter::Input_OnRotationMode()
{
	// 锁定期间强制 Aiming 旋转模式，切换按键暂不生效
	if (IsLockedOn())
	{
		return;
	}

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

// ===== 锁定系统（逻辑在 UPP_LockOnComponent，这里只保留转发） =====

void APP_PlayerCharacter::Input_OnLockOn()
{
	if (LockOnComponent)
	{
		LockOnComponent->ToggleLockOn();
	}
}

void APP_PlayerCharacter::EquipLongsword()
{
	APP_PlayerState* PS = GetPlayerState<APP_PlayerState>();
	if (!PS)
	{
		return;
	}
	if (UPP_InventoryComponent* Inventory = PS->GetInventoryComponent())
	{
		Inventory->Server_EquipItem(TEXT("Longsword"));
	}
}

FVector APP_PlayerCharacter::GetLockOnFocusLocation() const
{
	return LockOnComponent ? LockOnComponent->GetLockOnFocusLocation() : FVector::ZeroVector;
}

// ===== IPP_EquipmentUser 实现（武器装备 → GA 切换） =====

void APP_PlayerCharacter::OnEquipmentChanged_Implementation(EPP_EquipSlot Slot, FName /*NewItemID*/)
{
	// 只做转发：外观与 GA 各自从背包的装备数据拉取（不再把 NewItemID 往下传，避免两份数据源）
	// 当前只有主手武器槽参与外观 / 技能切换
	if (Slot != EPP_EquipSlot::MainHand)
	{
		return;
	}

	if (IsValid(WeaponVisualComponent))
	{
		WeaponVisualComponent->RefreshVisualFromInventory();
	}

	// GA 的授予/移除由组件内部判服务器权威，客户端调用直接返回
	if (IsValid(WeaponAbilityComponent))
	{
		WeaponAbilityComponent->RefreshAbilityFromInventory();
	}
}

void APP_PlayerCharacter::RestoreEquippedWeaponAbility()
{
	if (IsValid(WeaponAbilityComponent))
	{
		WeaponAbilityComponent->RefreshAbilityFromInventory();
	}
}

