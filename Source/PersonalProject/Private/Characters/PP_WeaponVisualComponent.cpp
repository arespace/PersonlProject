#include "Characters/PP_WeaponVisualComponent.h"

#include "Characters/PP_PlayerCharacter.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Data/PP_ItemTypes.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_Log.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Materials/MaterialInterface.h"
#include "Player/PP_PlayerState.h"

UPP_WeaponVisualComponent::UPP_WeaponVisualComponent()
{
	// 不需要每帧更新，外观只在装备变化时刷新
	PrimaryComponentTick.bCanEverTick = false;

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->SetStaticMesh(nullptr);
}

void UPP_WeaponVisualComponent::BeginPlay()
{
	Super::BeginPlay();

	InitializeVisual();

	// 组件的 BeginPlay 在角色 PossessedBy 之后才跑，这里必须按背包的实际装备刷新一次：
	// 空手 → 露出自带武器；已装备 → 隐藏自带武器并显示武器网格。不能再无条件隐藏，
	// 否则会把 PossessedBy 里刚恢复好的"空手显示自带武器"覆盖掉。
	RefreshVisualFromInventory();
}

void UPP_WeaponVisualComponent::InitializeVisual()
{
	USkeletalMeshComponent* SkeletalMesh = IsValid(GetOwner()) ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;

	// 把武器网格容器挂到配置的骨骼/socket 上（蓝图里改 WeaponAttachSocketName 后运行时生效）
	AttachToSocket(SkeletalMesh, WeaponMesh, WeaponAttachSocketName,
		FVector::ZeroVector, FRotator::ZeroRotator, FVector::OneVector);

	// 未手动配置材质槽时，按材质名自动识别自带武器（M_Kwang_Weapon_Bright / M05_Weapon 等）
	if (BuiltInWeaponMaterialSlots.Num() == 0)
	{
		AutoDetectMaterialSlots(SkeletalMesh, BuiltInWeaponMaterialSlots);
	}

	// 缓存自带武器材质槽的原始材质，供后续还原
	CachedBuiltInWeaponMaterials.Reset();
	if (IsValid(SkeletalMesh))
	{
		for (int32 SlotIndex : BuiltInWeaponMaterialSlots)
		{
			CachedBuiltInWeaponMaterials.Add(SkeletalMesh->GetMaterial(SlotIndex));
		}
	}
}

void UPP_WeaponVisualComponent::RefreshVisualFromInventory()
{
	APP_PlayerCharacter* Character = IsValid(GetOwner()) ? Cast<APP_PlayerCharacter>(GetOwner()) : nullptr;
	APP_PlayerState* PPPlayerState = IsValid(Character) ? Character->GetPlayerState<APP_PlayerState>() : nullptr;
	UPP_InventoryComponent* Inventory = IsValid(PPPlayerState) ? PPPlayerState->GetInventoryComponent() : nullptr;

	// 单武器槽口径：只看主手；空手 → 清网格并露出自带武器
	const FName WeaponID = IsValid(Inventory) ? Inventory->GetEquippedItemID(EPP_EquipSlot::MainHand) : NAME_None;
	if (WeaponID.IsNone())
	{
		ClearWeaponMesh();
		SetBuiltInWeaponVisible(true);
		return;
	}

	SetBuiltInWeaponVisible(false);
	SetWeaponMesh(WeaponID);
}

void UPP_WeaponVisualComponent::SetWeaponMesh(const FName ItemID)
{
	if (!IsValid(WeaponMesh))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] WeaponMesh 组件无效（组件未正确构造），无法显示 %s"), *ItemID.ToString());
		return;
	}

	// 唯一数据源：DT_Items 行内嵌的 WeaponConfig
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	const FPP_WeaponConfig* WeaponConfig = DataManager ? DataManager->GetWeaponConfig(ItemID) : nullptr;
	if (!WeaponConfig)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] ItemID=%s 在 DT_Items 中没有 WeaponConfig，武器无外观"), *ItemID.ToString());
		WeaponMesh->SetStaticMesh(nullptr);
		return;
	}

	const FPP_WeaponVisual& Visual = WeaponConfig->Visual;
	UStaticMesh* LoadedMesh = Visual.Mesh.LoadSynchronous();
	const FName SocketName = Visual.AttachSocket.IsNone() ? WeaponAttachSocketName : Visual.AttachSocket;

	USkeletalMeshComponent* SkeletalMesh = IsValid(GetOwner()) ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
	AttachToSocket(SkeletalMesh, WeaponMesh, SocketName,
		Visual.RelativeLocation, Visual.RelativeRotation, Visual.RelativeScale);

	WeaponMesh->SetStaticMesh(LoadedMesh);
	if (!LoadedMesh)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] DT_Items 行 %s 的 WeaponConfig.Visual.Mesh 为空，武器无外观"), *ItemID.ToString());
	}
}

void UPP_WeaponVisualComponent::ClearWeaponMesh()
{
	if (IsValid(WeaponMesh))
	{
		WeaponMesh->SetStaticMesh(nullptr);
	}
}

void UPP_WeaponVisualComponent::SetBuiltInWeaponVisible(const bool bVisible)
{
	USkeletalMeshComponent* SkeletalMesh = IsValid(GetOwner()) ? GetOwner()->FindComponentByClass<USkeletalMeshComponent>() : nullptr;
	if (!IsValid(SkeletalMesh))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponVisual] SetBuiltInWeaponVisible(%d): 骨骼网格组件无效"), bVisible ? 1 : 0);
		return;
	}

	// 网格可能在 BeginPlay 之后才被角色外观逻辑赋值，这里做一次惰性识别
	if (BuiltInWeaponMaterialSlots.Num() == 0)
	{
		AutoDetectMaterialSlots(SkeletalMesh, BuiltInWeaponMaterialSlots);
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
				Material == HiddenWeaponMaterial.Get() ? GetSkeletalMeshAssetMaterial(SkeletalMesh, SlotIndex) : Material);
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
			RestoreMaterial = GetSkeletalMeshAssetMaterial(SkeletalMesh, SlotIndex);
		}

		UMaterialInterface* AppliedMaterial = bVisible ? RestoreMaterial : HiddenWeaponMaterial.Get();
		SkeletalMesh->SetMaterial(SlotIndex, AppliedMaterial);
	}
}

void UPP_WeaponVisualComponent::AttachToSocket(USkeletalMeshComponent* SkeletalMesh, UStaticMeshComponent* WeaponMeshComponent,
	const FName SocketName, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& RelativeScale)
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

	// 已经挂在同一 socket / 骨骼上就不重复挂载（刷新入口可能被多次调用）
	const bool bAlreadyAttached = WeaponMeshComponent->GetAttachParent() == SkeletalMesh &&
		WeaponMeshComponent->GetAttachSocketName() == ResolvedSocket;
	if (!bAlreadyAttached)
	{
		WeaponMeshComponent->AttachToComponent(SkeletalMesh,
			FAttachmentTransformRules::SnapToTargetNotIncludingScale, ResolvedSocket);
	}

	// 模型枢轴与骨骼方向不一致时，用相对变换做修正
	WeaponMeshComponent->SetRelativeLocationAndRotation(RelativeLocation, RelativeRotation);
	WeaponMeshComponent->SetRelativeScale3D(RelativeScale);
}

void UPP_WeaponVisualComponent::AutoDetectMaterialSlots(USkeletalMeshComponent* SkeletalMesh, TArray<int32>& OutSlots)
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

UMaterialInterface* UPP_WeaponVisualComponent::GetSkeletalMeshAssetMaterial(USkeletalMeshComponent* SkeletalMesh, const int32 SlotIndex)
{
	if (!IsValid(SkeletalMesh)) return nullptr;

	const USkeletalMesh* Asset = SkeletalMesh->GetSkeletalMeshAsset();
	if (!IsValid(Asset)) return nullptr;

	const TArray<FSkeletalMaterial>& Materials = Asset->GetMaterials();
	return Materials.IsValidIndex(SlotIndex) ? Materials[SlotIndex].MaterialInterface.Get() : nullptr;
}
