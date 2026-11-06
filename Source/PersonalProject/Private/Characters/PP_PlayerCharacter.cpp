
#include "Characters/PP_PlayerCharacter.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "AlsAnimationInstance.h"
#include "AlsCameraComponent.h"
#include "AlsCameraSettings.h"
#include "Animation/AnimInstance.h"
#include "Camera/PlayerCameraManager.h"
#include "Characters/PP_EnemyCharacter.h"
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

	LockOnKey = EKeys::Q;

	// 默认掉落魂用 C++ 占位类，蓝图里可替换成自定义 BP
	SoulPickupClass = APP_SoulPickupActor::StaticClass();

	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.f);

	Tags.Add(PPActorTags::Player);

	// ===== 装备武器网格容器（挂在骨骼/socket 上，默认无网格） =====
	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh());
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetStaticMesh(nullptr);


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

// ===== 武器外观（自带烘焙武器 / 装备武器网格） =====

namespace
{
	/** 按材质名自动识别自带武器材质槽（网格可能在 BeginPlay 之后才被角色外观逻辑替换，需惰性调用） */
	void PP_AutoDetectWeaponMaterialSlots(USkeletalMeshComponent* SkeletalMesh, TArray<int32>& OutSlots)
	{
		if (!IsValid(SkeletalMesh)) return;

		for (int32 MaterialIndex = 0; MaterialIndex < SkeletalMesh->GetNumMaterials(); ++MaterialIndex)
		{
			const UMaterialInterface* Material = SkeletalMesh->GetMaterial(MaterialIndex);
			if (IsValid(Material) && Material->GetName().Contains(TEXT("Weapon"), ESearchCase::IgnoreCase))
			{
				OutSlots.Add(MaterialIndex);
			}
		}
	}

	/**
	 * 把武器网格挂到 socket 上。
	 * 配置的 socket 不存在时按 Paragon 通用挂点顺序回退（ik_hand_gun 是 Epic 给武器预留的挂点）；
	 * 网格资产还没就绪时一个都查不到，直接返回，等下次应用外观时再挂。
	 */
	void PP_AttachWeaponMeshToSocket(USkeletalMeshComponent* SkeletalMesh, UStaticMeshComponent* WeaponMeshComponent,
		FName SocketName, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& RelativeScale)
	{
		if (!IsValid(SkeletalMesh) || !IsValid(WeaponMeshComponent)) return;

		FName ResolvedSocket = SocketName;
		if (SkeletalMesh->DoesSocketExist(ResolvedSocket))
		{
			// 配置的挂点可用，直接使用
		}
		else
		{
			// 注意：不能声明为 static，否则 Live Coding 打补丁后不会重新初始化，仍用旧顺序
			const FName FallbackSockets[]
			{
				FName{TEXTVIEW("hand_r")},
				FName{TEXTVIEW("ik_hand_gun")},
				FName{TEXTVIEW("weapon_r")}
			};

			// DoesSocketExist 只认 socket，不认骨骼；hand_r 这类真实手骨必须用 GetBoneIndex 判定
			const auto IsValidAttachTarget = [SkeletalMesh](const FName& Candidate)
			{
				return SkeletalMesh->DoesSocketExist(Candidate) || SkeletalMesh->GetBoneIndex(Candidate) != INDEX_NONE;
			};

			ResolvedSocket = NAME_None;
			for (const FName& Candidate : FallbackSockets)
			{
				if (IsValidAttachTarget(Candidate))
				{
					ResolvedSocket = Candidate;
					break;
				}
			}

			// 精确候选都不存在时，模糊匹配真实右手骨骼（排除 ik_ / VB_ 这些辅助骨骼）
			if (ResolvedSocket.IsNone())
			{
				TArray<FName> BoneNames;
				SkeletalMesh->GetBoneNames(BoneNames);
				for (const FName& BoneName : BoneNames)
				{
					const FString NameStr = BoneName.ToString();
					if (NameStr.StartsWith(TEXT("ik_"), ESearchCase::IgnoreCase) ||
						NameStr.StartsWith(TEXT("VB_"), ESearchCase::IgnoreCase))
					{
						continue;
					}
					if (NameStr.Contains(TEXT("hand"), ESearchCase::IgnoreCase) &&
						NameStr.EndsWith(TEXT("_r"), ESearchCase::IgnoreCase))
					{
						ResolvedSocket = BoneName;
						break;
					}
				}
			}

			if (ResolvedSocket.IsNone())
			{
				UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] 骨骼上没有 socket '%s'，通用挂点也不存在（网格资产可能还没就绪），本次不挂载"),
					*SocketName.ToString());
				return;
			}
		}

		WeaponMeshComponent->AttachToComponent(SkeletalMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, ResolvedSocket);
		// 模型枢轴与骨骼方向不一致时，用相对变换做修正
		WeaponMeshComponent->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
		WeaponMeshComponent->SetRelativeScale3D(RelativeScale);
	}

	/** 从网格资产取原始材质（不依赖运行时缓存，网格被替换后也能正确还原） */
	UMaterialInterface* PP_GetSkeletalMeshAssetMaterial(USkeletalMeshComponent* SkeletalMesh, int32 SlotIndex)
	{
		if (!IsValid(SkeletalMesh)) return nullptr;

		const USkeletalMesh* Asset = SkeletalMesh->GetSkeletalMeshAsset();
		if (!IsValid(Asset)) return nullptr;

		const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
		return Materials.IsValidIndex(SlotIndex) ? Materials[SlotIndex].MaterialInterface.Get() : nullptr;
	}
}

void APP_PlayerCharacter::BeginPlay()
{
	Super::BeginPlay();

	// 把武器网格容器挂到配置的骨骼/socket 上（蓝图里改 WeaponAttachSocketName 后运行时生效）
	PP_AttachWeaponMeshToSocket(GetMesh(), WeaponMesh, WeaponAttachSocketName,
		FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector);

	// 未手动配置材质槽时，按材质名自动识别自带武器（M_Kwang_Weapon_Bright / M05_Weapon 等）
	if (BuiltInWeaponMaterialSlots.Num() == 0)
	{
		PP_AutoDetectWeaponMaterialSlots(GetMesh(), BuiltInWeaponMaterialSlots);
	}

	// 缓存自带武器材质槽的原始材质，供后续还原
	CachedBuiltInWeaponMaterials.Reset();
	if (IsValid(GetMesh()))
	{
		for (int32 SlotIndex : BuiltInWeaponMaterialSlots)
		{
			CachedBuiltInWeaponMaterials.Add(GetMesh()->GetMaterial(SlotIndex));
		}
	}

	if (bHideBuiltInWeaponOnBeginPlay)
	{
		SetBuiltInWeaponVisible(false);
	}
}

void APP_PlayerCharacter::SetBuiltInWeaponVisible(bool bVisible)
{
	USkeletalMeshComponent* SkeletalMesh = GetMesh();
	if (!IsValid(SkeletalMesh))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] SetBuiltInWeaponVisible(%d): 骨骼网格组件无效"), bVisible ? 1 : 0);
		return;
	}

	// 网格可能在 BeginPlay 之后才被角色外观逻辑赋值，这里做一次惰性识别
	if (BuiltInWeaponMaterialSlots.Num() == 0)
	{
		PP_AutoDetectWeaponMaterialSlots(SkeletalMesh, BuiltInWeaponMaterialSlots);
	}

	// 缓存缺失时（BeginPlay 时网格还没有材质）补录
	if (CachedBuiltInWeaponMaterials.Num() != BuiltInWeaponMaterialSlots.Num())
	{
		CachedBuiltInWeaponMaterials.Reset();
		for (int32 SlotIndex : BuiltInWeaponMaterialSlots)
		{
			UMaterialInterface* Material = SkeletalMesh->GetMaterial(SlotIndex);
			// 已经是隐藏材质时不要缓存，改用网格资产里的原始材质
			CachedBuiltInWeaponMaterials.Add(
				Material == HiddenWeaponMaterial.Get() ? PP_GetSkeletalMeshAssetMaterial(SkeletalMesh, SlotIndex) : Material);
		}
	}

	if (BuiltInWeaponMaterialSlots.Num() == 0)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] SetBuiltInWeaponVisible(%d): 未识别到自带武器材质槽"), bVisible ? 1 : 0);
		return;
	}

	if (!bVisible && !IsValid(HiddenWeaponMaterial))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] HiddenWeaponMaterial 未配置，无法隐藏自带武器"));
		return;
	}

	for (int32 Index = 0; Index < BuiltInWeaponMaterialSlots.Num(); ++Index)
	{
		const int32 SlotIndex = BuiltInWeaponMaterialSlots[Index];
		UMaterialInterface* RestoreMaterial = CachedBuiltInWeaponMaterials.IsValidIndex(Index)
			? CachedBuiltInWeaponMaterials[Index].Get()
			: nullptr;

		// 缓存拿不到时回退到网格资产自带的材质
		if (!IsValid(RestoreMaterial))
		{
			RestoreMaterial = PP_GetSkeletalMeshAssetMaterial(SkeletalMesh, SlotIndex);
		}

		UMaterialInterface* AppliedMaterial = bVisible ? RestoreMaterial : HiddenWeaponMaterial.Get();
		SkeletalMesh->SetMaterial(SlotIndex, AppliedMaterial);
	}
}

void APP_PlayerCharacter::SetWeaponMesh(FName ItemID)
{
	if (!IsValid(WeaponMesh)) return;

	const FPP_WeaponMeshVisual* Visual = WeaponMeshMap.Find(ItemID);
	if (!Visual)
	{
		FString ExistingKeys;
		for (const TPair<FName, FPP_WeaponMeshVisual>& Pair : WeaponMeshMap)
		{
			ExistingKeys += FString::Printf(TEXT("'%s' "), *Pair.Key.ToString());
		}

		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] WeaponMeshMap 未找到 ItemID='%s'（Map 当前 %d 条，已有键=[%s]）"),
			*ItemID.ToString(), WeaponMeshMap.Num(), *ExistingKeys);
		WeaponMesh->SetStaticMesh(nullptr);
		return;
	}

	// 挂点在 BeginPlay 时可能查不到（网格资产还没就绪），这里再挂一次兜底，并应用该武器自己的变换
	PP_AttachWeaponMeshToSocket(GetMesh(), WeaponMesh, WeaponAttachSocketName,
		Visual->RelativeLocation, Visual->RelativeRotation, Visual->RelativeScale);

	WeaponMesh->SetStaticMesh(Visual->Mesh);
}

void APP_PlayerCharacter::ClearWeaponMesh()
{
	if (IsValid(WeaponMesh))
	{
		WeaponMesh->SetStaticMesh(nullptr);
	}
}

void APP_PlayerCharacter::DebugTogglePrimaryWeapon()
{
	constexpr EPP_EquipmentSlotType SlotType = EPP_EquipmentSlotType::PrimaryWeapon;

	APP_PlayerState* PPPlayerState = Cast<APP_PlayerState>(GetPlayerState());
	UPP_InventoryComponent* Inventory = IsValid(PPPlayerState) ? PPPlayerState->GetInventoryComponent() : nullptr;
	if (!IsValid(Inventory))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponDebug] F9: 取不到 InventoryComponent，无法切换主武器"));
		return;
	}

	const FPP_EquipmentSlot* CurrentSlot = Inventory->GetEquipmentSlots().FindByPredicate(
		[SlotType](const FPP_EquipmentSlot& Candidate) { return Candidate.SlotType == SlotType; });
	const FName CurrentItemID = CurrentSlot ? CurrentSlot->ItemID : NAME_None;

	// 已装备 → 卸下；未装备 → 补一件再装上
	if (!CurrentItemID.IsNone())
	{
		Inventory->Server_UnequipWeapon(SlotType);
		return;
	}

	if (Inventory->GetItemCount(DebugPrimaryWeaponItemID) <= 0)
	{
		if (HasAuthority())
		{
			GrantItemToInventory(DebugPrimaryWeaponItemID, 1);
		}
		else
		{
			ServerPPAddItem(DebugPrimaryWeaponItemID, 1);
		}
	}

	Inventory->Server_EquipWeapon(DebugPrimaryWeaponItemID, SlotType);
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

	UPP_AttributeSet* PP_AttributeSet = Cast<UPP_AttributeSet>(GetAttributeSet());
	if (!IsValid(PP_AttributeSet)) return;

	// 先 RemoveAll 再 Add：监听服务器上 PossessedBy 与 OnRep_PlayerState 都会执行，
	// 直接 AddUObject 会重复绑定，导致一次掉血触发两次 OnHealthChanged → HandleDeath 执行两次
	// （日志里出现 "bAlive=1" 和 "bAlive=0" 两条服务端死亡记录）。
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).RemoveAll(this);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetStaminaAttribute()).RemoveAll(this);
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

	// 同上：避免 PossessedBy / OnRep_PlayerState 重复绑定
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).RemoveAll(this);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetHealthAttribute()).AddUObject(this, &ThisClass::OnHealthChanged);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetStaminaAttribute()).RemoveAll(this);
	GetAbilitySystemComponent()->GetGameplayAttributeValueChangeDelegate(PP_AttributeSet->GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);
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
	UpdateLockOn(DeltaTime);

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
	// 基类负责：置死亡标记、Ragdoll、停移动、禁输入、启动复活计时
	Super::HandleDeath();

	if (!HasAuthority() || !GetWorld())
	{
		return;
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

		// 锁定键走 legacy BindKey：无需额外 IA/IMC 资产，按键可在玩家蓝图里改
		if (LockOnKey.IsValid())
		{
			Input->BindKey(LockOnKey, IE_Pressed, this, &ThisClass::Input_OnLockOn);
		}

		// 武器外观调试键：F9 切换主武器（F 已被攻击技能占用，不能复用）
		Input->BindKey(EKeys::F9, IE_Pressed, this, &ThisClass::DebugTogglePrimaryWeapon);
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

// ===== 锁定系统 =====

void APP_PlayerCharacter::Input_OnLockOn()
{
	ToggleLockOn();
}

void APP_PlayerCharacter::ToggleLockOn()
{
	if (!IsLocallyControlled())
	{
		return;
	}

	if (!IsAlive())
	{
		ReleaseLockOn();
		return;
	}

	// 已有锁定则解锁
	if (LockOnTarget.IsValid())
	{
		ReleaseLockOn();
		return;
	}

	if (AActor* Found = FindLockOnTarget())
	{
		LockOnTarget = Found;
		PreLockOnRotationMode = GetDesiredRotationMode();

		// Aiming 旋转模式让角色在静止时也朝向视角（即目标），
		// 移动时则保持下半身朝目标、可横向绕圈。
		SetDesiredRotationMode(AlsRotationModeTags::Aiming);

		UE_LOG(LogPP, Log, TEXT("[LockOn] 锁定目标: %s"), *Found->GetName());
	}
}

FVector APP_PlayerCharacter::GetLockOnFocusLocation() const
{
	const AActor* Target = LockOnTarget.Get();
	return IsValid(Target)
		       ? Target->GetActorLocation() + FVector(0.f, 0.f, LockOnFocusHeight)
		       : FVector::ZeroVector;
}

bool APP_PlayerCharacter::IsLockOnTargetValid(const AActor* Target) const
{
	if (!IsValid(Target))
	{
		return false;
	}

	const APP_BaseCharacter* TargetCharacter = Cast<APP_BaseCharacter>(Target);
	if (!TargetCharacter || !TargetCharacter->IsAlive())
	{
		return false;
	}

	return FVector::DistSquared(GetActorLocation(), Target->GetActorLocation())
		<= FMath::Square(LockOnBreakDistance);
}

AActor* APP_PlayerCharacter::FindLockOnTarget()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return nullptr;
	}

	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(LockOnMaxScreenAngle));

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (TActorIterator<APP_EnemyCharacter> It(GetWorld()); It; ++It)
	{
		APP_EnemyCharacter* Enemy = *It;
		if (!IsValid(Enemy) || !Enemy->IsAlive() || Enemy->IsHidden())
		{
			continue;
		}

		const float Distance = FVector::Dist(GetActorLocation(), Enemy->GetActorLocation());
		if (Distance > LockOnSearchRadius)
		{
			continue;
		}

		const FVector FocusLocation = Enemy->GetActorLocation() + FVector(0.f, 0.f, LockOnFocusHeight);
		const FVector ToTarget = FocusLocation - CameraLocation;
		const float Dot = FVector::DotProduct(CameraForward, ToTarget.GetSafeNormal());
		if (Dot < MinDot)
		{
			continue;
		}

		// 视线校验：忽略自身与目标本身，任何遮挡都视为不可见
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PP_LockOnTrace), false, this);
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(Enemy);

		FHitResult Hit;
		if (GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, FocusLocation, ECC_Visibility, Params))
		{
			continue;
		}

		// 综合距离与屏幕角度：越近、越靠近画面中心分越高
		const float AnglePenalty = FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)) / PI;
		const float Score = Distance / LockOnSearchRadius + AnglePenalty;
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

void APP_PlayerCharacter::UpdateLockOn(float DeltaTime)
{
	if (!IsLockedOn())
	{
		return;
	}

	if (!IsAlive() || !IsLockOnTargetValid(LockOnTarget.Get()))
	{
		ReleaseLockOn();
		return;
	}

	if (!IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return;
	}

	// 相机与角色朝向都由 ControlRotation 驱动：ALS 会把本地视角同步到服务端，
	// 服务端因此能据此计算角色朝向，攻击命中判定方向与画面一致。
	const FVector FocusLocation = GetLockOnFocusLocation();
	const FRotator DesiredRotation = (FocusLocation - PC->PlayerCameraManager->GetCameraLocation()).Rotation();
	const FRotator CurrentRotation = PC->GetControlRotation();

	PC->SetControlRotation(FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, LockOnCameraInterpolationSpeed));
}

void APP_PlayerCharacter::ReleaseLockOn()
{
	if (!LockOnTarget.IsValid() && !PreLockOnRotationMode.IsValid())
	{
		return;
	}

	const bool bWasLocked = LockOnTarget.IsValid();
	LockOnTarget.Reset();

	if (PreLockOnRotationMode.IsValid())
	{
		if (IsLocallyControlled())
		{
			UE_LOG(LogPP, Log, TEXT("[LockOn] 恢复旋转模式: %s → %s"),
				*GetDesiredRotationMode().ToString(), *PreLockOnRotationMode.ToString());
			SetDesiredRotationMode(PreLockOnRotationMode);
		}
		PreLockOnRotationMode = FGameplayTag();
	}

	if (bWasLocked)
	{
		UE_LOG(LogPP, Log, TEXT("[LockOn] 解除锁定"));
	}
}

// ===== IPP_EquipmentUser 实现（武器装备 → GA 切换） =====

void APP_PlayerCharacter::OnEquipmentChanged_Implementation(EPP_EquipmentSlotType SlotType, FName NewItemID)
{
	// 先同步武器外观：主手枪位为空 → 露出自带武器；有武器 → 隐藏自带武器并显示对应网格
	if (SlotType == EPP_EquipmentSlotType::PrimaryWeapon)
	{
		if (NewItemID.IsNone())
		{
			ClearWeaponMesh();
			SetBuiltInWeaponVisible(true);
		}
		else
		{
			SetBuiltInWeaponVisible(false);
			SetWeaponMesh(NewItemID);
		}
	}

	// 以下只属于服务器：GA 的授予/移除必须由权威端执行（外观部分已在上方双端完成）
	if (!HasAuthority())
	{
		return;
	}

	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] OnEquipmentChanged: ASC 无效，无法切换武器 GA"));
		return;
	}

	// 第一步：移除该槽位上一次激活的 GA
	if (const FGameplayAbilitySpecHandle* OldHandle = WeaponAbilityHandles.Find(SlotType))
	{
		if (OldHandle->IsValid())
		{
			ASC->ClearAbility(*OldHandle);
			WeaponAbilityHandles.Remove(SlotType);
		}
	}

	// 第二步：如果 NewItemID 有效 → 查 WeaponData → Give 新 GA
	if (NewItemID.IsNone())
	{
		// 卸下，到此结束
		return;
	}

	if (!IsValid(WeaponDataTable))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] WeaponDataTable 未配置，无法给 %s 装备 GA"), *NewItemID.ToString());
		return;
	}

	const FPP_WeaponData* WeaponRow = WeaponDataTable->FindRow<FPP_WeaponData>(NewItemID, TEXT("OnEquipmentChanged"));
	if (!WeaponRow)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] DT_WeaponData 中未找到 ItemID=%s"), *NewItemID.ToString());
		return;
	}

	if (!WeaponRow->WeaponAbilityClass)
	{
		// 该武器未配置 GA 类，跳过（纯属性武器）
		return;
	}

	// GiveAbility（Level 1，无 InputBinding）
	FGameplayAbilitySpec Spec(WeaponRow->WeaponAbilityClass, 1, INDEX_NONE, this);
	const FGameplayAbilitySpecHandle NewHandle = ASC->GiveAbility(Spec);
	if (NewHandle.IsValid())
	{
		WeaponAbilityHandles.Add(SlotType, NewHandle);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] 槽 %d 装备 %s → GiveAbility 失败"),
			static_cast<int32>(SlotType), *NewItemID.ToString());
	}
}

