// 库存系统 — 库存与装备管理组件实现
// 关键逻辑：堆叠合并、装备替换、GE施加/移除、网络复制

#include "Inventory/PP_InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Framework/PP_DataManager.h"
#include "Events/PP_EventBus.h"
#include "Events/PP_EventTypes.h"
#include "Framework/PP_Log.h"
#include "GameplayTags/PPTags.h"
#include "Interfaces/PP_EquipmentUserInterface.h"
#include "Interfaces/PP_InteractableInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"

namespace
{
	/** 主手/副手 */
	bool IsWeaponSlot(EPP_EquipSlot Slot)
	{
		return Slot == EPP_EquipSlot::MainHand || Slot == EPP_EquipSlot::OffHand;
	}
}

UPP_InventoryComponent::UPP_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Gold = 1000;
}

void UPP_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPP_InventoryComponent, Items);
	DOREPLIFETIME(UPP_InventoryComponent, EquippedItems);
	DOREPLIFETIME(UPP_InventoryComponent, Gold);
}

const FPP_ItemInfo* UPP_InventoryComponent::FindItemInfo(FName ItemID) const
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}
	// 统一走 DataManager 全局物品表（数据表 RowName 与 ItemID 保持一致，直接按 RowName 查）
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	if (!DataManager)
	{
		return nullptr;
	}
	return DataManager->GetItemInfo(ItemID);
}

// ===== 物品基础接口 =====

bool UPP_InventoryComponent::AddItem(FName ItemID, int32 Count)
{
	// 仅服务器执行
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (ItemID.IsNone() || Count <= 0)
	{
		return false;
	}
	const FPP_ItemInfo* Info = FindItemInfo(ItemID);
	if (!Info)
	{
		UE_LOG(LogPP, Warning, TEXT("[Inventory] AddItem: 未找到物品配置 %s"), *ItemID.ToString());
		return false;
	}

	const int32 MaxStack = FMath::Max(1, Info->MaxStackCount);
	int32 Remaining = Count;

	UE_LOG(LogPP, Log, TEXT("[Inventory] AddItem: ItemID=%s, Count=%d, MaxStack=%d (DataTable raw=%d), 当前背包槽位数=%d"),
		*ItemID.ToString(), Count, MaxStack, Info->MaxStackCount, Items.Num());

	// 1) 优先填充已存在的同ID堆叠槽
	for (FPP_ItemInstance& Inst : Items)
	{
		if (Remaining <= 0) break;
		if (Inst.ItemID == ItemID && Inst.Count < MaxStack)
		{
			const int32 Add = FMath::Min(Remaining, MaxStack - Inst.Count);
			Inst.Count += Add;
			Remaining -= Add;
			UE_LOG(LogPP, Log, TEXT("[Inventory] AddItem: 堆叠到已有槽位，槽位Count=%d -> %d，剩余=%d"),
				Inst.Count - Add, Inst.Count, Remaining);
		}
	}

	// 2) 剩余数量按 MaxStack 分块新建条目
	int32 NewSlotsCreated = 0;
	while (Remaining > 0)
	{
		if (MaxInventorySlots > 0 && Items.Num() >= MaxInventorySlots)
		{
			UE_LOG(LogPP, Warning, TEXT("[Inventory] AddItem: 背包已满，剩余 %d 件未能加入"), Remaining);
			break;
		}
		const int32 Add = FMath::Min(Remaining, MaxStack);
		Items.Emplace(ItemID, Add);
		Remaining -= Add;
		++NewSlotsCreated;
		UE_LOG(LogPP, Log, TEXT("[Inventory] AddItem: 新建槽位 #%d，Count=%d，剩余=%d"),
			NewSlotsCreated, Add, Remaining);
	}

	const int32 NewTotal = GetItemCount(ItemID);
	UE_LOG(LogPP, Log, TEXT("[Inventory] AddItem: 完成 — 物品=%s, 本次新建槽位=%d, 当前该物品总数=%d, 背包总槽位=%d"),
		*ItemID.ToString(), NewSlotsCreated, NewTotal, Items.Num());
	BroadcastInventoryUpdatedEvent();
	return Remaining < Count;
}

bool UPP_InventoryComponent::RemoveItem(FName ItemID, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (ItemID.IsNone() || Count <= 0)
	{
		return false;
	}
	if (GetItemCount(ItemID) < Count)
	{
		return false;
	}

	int32 Remaining = Count;
	// 倒序遍历方便就地删除
	for (int32 i = Items.Num() - 1; i >= 0 && Remaining > 0; --i)
	{
		FPP_ItemInstance& Inst = Items[i];
		if (Inst.ItemID != ItemID) continue;

		const int32 Take = FMath::Min(Remaining, Inst.Count);
		Inst.Count -= Take;
		Remaining -= Take;
		if (Inst.Count <= 0)
		{
			Items.RemoveAt(i);
		}
	}

	const int32 NewTotal = GetItemCount(ItemID);
	BroadcastInventoryUpdatedEvent();
	return true;
}

int32 UPP_InventoryComponent::GetItemCount(FName ItemID) const
{
	int32 Total = 0;
	for (const FPP_ItemInstance& Inst : Items)
	{
		if (Inst.ItemID == ItemID)
		{
			Total += Inst.Count;
		}
	}
	return Total;
}

bool UPP_InventoryComponent::HasItem(FName ItemID, int32 Count) const
{
	return GetItemCount(ItemID) >= FMath::Max(1, Count);
}

// ===== 装备接口 =====

void UPP_InventoryComponent::Server_EquipItem_Implementation(FName ItemID)
{
	EquipItemInternal(ItemID, EPP_EquipSlot::None);
}

void UPP_InventoryComponent::Server_UnequipItem_Implementation(EPP_EquipSlot Slot)
{
	UnequipItemInternal(Slot);
}

EPP_EquipSlot UPP_InventoryComponent::ResolveEquipSlot(const FPP_ItemInfo& ItemInfo, EPP_EquipSlot PreferredSlot) const
{
	if (ItemInfo.EquipSlot == EPP_EquipSlot::None)
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 物品 %s 未配置装备槽位，无法装备"), *ItemInfo.ItemID.ToString());
		return EPP_EquipSlot::None;
	}
	if (PreferredSlot == EPP_EquipSlot::None || PreferredSlot == ItemInfo.EquipSlot)
	{
		return ItemInfo.EquipSlot;
	}

	// 武器允许主/副手互换，但必须由数据表显式放行副手
	if (IsWeaponSlot(PreferredSlot) && IsWeaponSlot(ItemInfo.EquipSlot)
		&& PreferredSlot == EPP_EquipSlot::OffHand && ItemInfo.WeaponConfig.bCanUseOffHand)
	{
		return EPP_EquipSlot::OffHand;
	}

	UE_LOG(LogPP, Warning, TEXT("[Equipment] 物品 %s 的槽位 %d 与请求槽位 %d 不匹配，装备失败"),
		*ItemInfo.ItemID.ToString(), static_cast<int32>(ItemInfo.EquipSlot), static_cast<int32>(PreferredSlot));
	return EPP_EquipSlot::None;
}

FActiveGameplayEffectHandle UPP_InventoryComponent::ApplyEquipEffect(const FPP_ItemInfo& ItemInfo)
{
	// 未配置属性效果属正常情况（纯技能武器 / 纯外观装备）
	if (!ItemInfo.EquipEffectClass)
	{
		return FActiveGameplayEffectHandle();
	}

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 装备 %s：ASC 无效，属性效果未施加"), *ItemInfo.ItemID.ToString());
		return FActiveGameplayEffectHandle();
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemInfo.EquipEffectClass, 1.0f, Context);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 装备 %s：EquipEffectClass 无法生成 Spec"), *ItemInfo.ItemID.ToString());
		return FActiveGameplayEffectHandle();
	}
	return ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UPP_InventoryComponent::EquipItemInternal(FName ItemID, EPP_EquipSlot PreferredSlot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (ItemID.IsNone())
	{
		return;
	}

	const FPP_ItemInfo* ItemInfo = FindItemInfo(ItemID);
	if (!ItemInfo || ItemInfo->ItemType != EPP_ItemType::Equipment || ItemInfo->EquipSlot == EPP_EquipSlot::None)
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 物品 %s 不是可装备物品，装备失败"), *ItemID.ToString());
		return;
	}

	const EPP_EquipSlot TargetSlot = ResolveEquipSlot(*ItemInfo, PreferredSlot);
	if (TargetSlot == EPP_EquipSlot::None)
	{
		return;
	}

	// 相同物品重复装备直接短路，避免一次操作产生双 GA / 双 GE
	if (FindEquippedItemID(TargetSlot) == ItemID)
	{
		return;
	}

	// 新装备必须先在背包里（防止凭空装备）
	if (!HasItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 背包中没有 %s，装备失败"), *ItemID.ToString());
		return;
	}
	if (!RemoveItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 扣减背包 %s 失败"), *ItemID.ToString());
		return;
	}

	// 同槽旧装备回背包；背包满则整次换装回滚，避免物品凭空消失
	const FName OldItemID = FindEquippedItemID(TargetSlot);
	if (!OldItemID.IsNone())
	{
		if (!AddItem(OldItemID, 1))
		{
			AddItem(ItemID, 1);
			UE_LOG(LogPP, Warning, TEXT("[Equipment] 背包已满，无法收回旧装备 %s，换装取消"), *OldItemID.ToString());
			return;
		}
		RemoveEquippedSlot(TargetSlot);
	}

	FPP_EquippedItem NewEquip;
	NewEquip.ItemID = ItemID;
	NewEquip.Slot = TargetSlot;
	NewEquip.EffectHandle = ApplyEquipEffect(*ItemInfo);
	EquippedItems.Add(NewEquip);

	// 通知 UI 刷新装备栏（服务端不会触发 OnRep_Equipped）
	BroadcastEquipmentUpdatedEvent();
	NotifyEquipmentUser(TargetSlot, ItemID);
}

void UPP_InventoryComponent::UnequipItemInternal(EPP_EquipSlot Slot)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (Slot == EPP_EquipSlot::None)
	{
		return;
	}

	const FName ItemID = FindEquippedItemID(Slot);
	if (ItemID.IsNone())
	{
		return;
	}

	// 卸下物品回背包；背包满则取消卸下
	if (!AddItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 背包已满，无法收回 %s，卸下取消"), *ItemID.ToString());
		return;
	}

	RemoveEquippedSlot(Slot);

	BroadcastEquipmentUpdatedEvent();
	NotifyEquipmentUser(Slot, NAME_None);
}

void UPP_InventoryComponent::RemoveEquippedSlot(EPP_EquipSlot Slot)
{
	const int32 Idx = EquippedItems.IndexOfByPredicate([Slot](const FPP_EquippedItem& E)
	{
		return E.Slot == Slot;
	});

	if (Idx != INDEX_NONE)
	{
		// 通过句柄精确移除 GE
		if (EquippedItems[Idx].EffectHandle.IsValid())
		{
			if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
			{
				ASC->RemoveActiveGameplayEffect(EquippedItems[Idx].EffectHandle);
			}
		}
		EquippedItems.RemoveAt(Idx);
	}
}

void UPP_InventoryComponent::NotifyEquipmentUser(EPP_EquipSlot Slot, FName NewItemID)
{
	AActor* NotifyTarget = GetEquipmentUserActor();
	if (!NotifyTarget || !NotifyTarget->GetClass()->ImplementsInterface(UPP_EquipmentUser::StaticClass()))
	{
		UE_LOG(LogPP, Warning, TEXT("[Equipment] 槽 %d 变更：未找到装备使用者 Actor（Pawn 为空或未实现 IPP_EquipmentUser），外观/技能不会切换"),
			static_cast<int32>(Slot));
		return;
	}

	// BlueprintNativeEvent 不能直接调用，必须走 Execute_ 包装
	IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, Slot, NewItemID);
}

FName UPP_InventoryComponent::FindEquippedItemID(EPP_EquipSlot Slot) const
{
	for (const FPP_EquippedItem& E : EquippedItems)
	{
		if (E.Slot == Slot)
		{
			return E.ItemID;
		}
	}
	return NAME_None;
}

FName UPP_InventoryComponent::GetEquippedItemID(EPP_EquipSlot Slot) const
{
	return GetEquippedItemInSlot(Slot).ItemID;
}

UAbilitySystemComponent* UPP_InventoryComponent::GetOwningAbilitySystemComponent() const
{
	// 优先从挂载的 PlayerState 获取
	if (APlayerState* PS = Cast<APlayerState>(GetOwner()))
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(PS);
	}
	// 兜底：从 Pawn / Controller 解析
	if (AActor* Owner = GetOwner())
	{
		return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Owner);
	}
	return nullptr;
}

AActor* UPP_InventoryComponent::GetEquipmentUserActor() const
{
	if (AActor* Owner = GetOwner())
	{
		// Owner 本身就是 PlayerCharacter 或实现了接口 → 直接返回
		if (Owner->GetClass()->ImplementsInterface(UPP_EquipmentUser::StaticClass()))
		{
			return Owner;
		}
		// Owner 是 PlayerState → 尝试取对应的 Pawn（PlayerCharacter 挂在 Pawn 上）
		if (APlayerState* PS = Cast<APlayerState>(Owner))
		{
			if (APawn* Pawn = PS->GetPawn())
			{
				return Pawn;
			}
		}
	}
	return nullptr;
}

// ===== 货币接口 =====

bool UPP_InventoryComponent::SpendGold(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return false;
	}
	if (Amount <= 0 || Gold < Amount)
	{
		return false;
	}
	Gold -= Amount;
	BroadcastGoldUpdatedEvent(Gold);
	return true;
}

void UPP_InventoryComponent::AddGold(int32 Amount)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (Amount <= 0)
	{
		return;
	}
	Gold += Amount;
	BroadcastGoldUpdatedEvent(Gold);
}

// ===== OnRep 回调 =====

void UPP_InventoryComponent::OnRep_Items(const TArray<FPP_ItemInstance>& OldItems)
{
	// 客户端通用刷新：广播 EventBus 事件，UI 监听者据此整体重绘
	BroadcastInventoryUpdatedEvent();
}

void UPP_InventoryComponent::OnRep_Equipped(const TArray<FPP_EquippedItem>& OldEquipped)
{
	// 装备列表变化：触发 UI 重绘
	BroadcastEquipmentUpdatedEvent();

	// 客户端收到装备复制：SetMaterial / SetStaticMesh 不参与网络复制，
	// 因此外观（隐藏自带武器 / 显示装备武器网格）必须在客户端收到槽位变化后重新应用一次。
	AActor* NotifyTarget = GetEquipmentUserActor();
	if (!IsValid(NotifyTarget) || !NotifyTarget->GetClass()->ImplementsInterface(UPP_EquipmentUser::StaticClass()))
	{
		// 客户端 Pawn 可能还没生成，此时外观由角色 BeginPlay 的默认状态负责
		return;
	}

	// 找出所有发生变化的槽位（新增/替换/卸下），逐个通知
	auto FindOldItemID = [&OldEquipped](EPP_EquipSlot Slot) -> FName
	{
		const FPP_EquippedItem* Old = OldEquipped.FindByPredicate(
			[Slot](const FPP_EquippedItem& E) { return E.Slot == Slot; });
		return Old ? Old->ItemID : NAME_None;
	};

	for (const FPP_EquippedItem& Equipped : EquippedItems)
	{
		if (FindOldItemID(Equipped.Slot) != Equipped.ItemID)
		{
			IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, Equipped.Slot, Equipped.ItemID);
		}
	}

	// 处理被卸下的槽位：旧列表里有、新列表里没有 → 通知 NAME_None
	for (const FPP_EquippedItem& Old : OldEquipped)
	{
		const bool StillEquipped = EquippedItems.ContainsByPredicate(
			[&Old](const FPP_EquippedItem& E) { return E.Slot == Old.Slot; });
		if (!StillEquipped)
		{
			IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, Old.Slot, NAME_None);
		}
	}
}

void UPP_InventoryComponent::OnRep_Gold(int32 OldGold)
{
	BroadcastGoldUpdatedEvent(Gold);
}

void UPP_InventoryComponent::Server_UseConsumable_Implementation(FName ItemID)
{
	if (!HasItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[Inventory] UseConsumable: 背包中无物品 %s"), *ItemID.ToString());
		return;
	}
	const FPP_ItemInfo* Info = FindItemInfo(ItemID);
	if (!Info)
	{
		return;
	}
	if (Info->ItemType != EPP_ItemType::Consumable)
	{
		UE_LOG(LogPP, Warning, TEXT("[Inventory] UseConsumable: 物品 %s 不是消耗品"), *ItemID.ToString());
		return;
	}
	if (!Info->ConsumeEffectClass)
	{
		UE_LOG(LogPP, Warning, TEXT("[Inventory] UseConsumable: 物品 %s 未配置 ConsumeEffectClass"), *ItemID.ToString());
		return;
	}

	// 扣减1件
	if (!RemoveItem(ItemID, 1))
	{
		return;
	}

	// 施加 GE
	ApplyConsumable(*Info);

	UE_LOG(LogPP, Log, TEXT("[Inventory] UseConsumable: 使用 %s"), *ItemID.ToString());
}

void UPP_InventoryComponent::ApplyConsumable(const FPP_ItemInfo& ItemInfo)
{
	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!ASC || !ItemInfo.ConsumeEffectClass)
	{
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemInfo.ConsumeEffectClass, 1.0f, Context);
	if (SpecHandle.IsValid())
	{
		ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

FPP_EquippedItem UPP_InventoryComponent::GetEquippedItemInSlot(EPP_EquipSlot Slot) const
{
	// 唯一数据源为 EquippedItems，武器与护甲同表
	for (const FPP_EquippedItem& E : EquippedItems)
	{
		if (E.Slot == Slot)
		{
			return E;
		}
	}
	return FPP_EquippedItem();
}

// ===== IPP_InventoryQuery 接口实现 =====

TArray<FPP_ItemInstance> UPP_InventoryComponent::QueryItems_Implementation()
{
	return Items;
}

TArray<FPP_EquippedItem> UPP_InventoryComponent::QueryEquippedItems_Implementation()
{
	// B2 起单一数据源：EquippedItems 同时包含武器与护甲
	return EquippedItems;
}

int32 UPP_InventoryComponent::QueryGold_Implementation()
{
	return Gold;
}

int32 UPP_InventoryComponent::QueryItemCount_Implementation(FName ItemID)
{
	return GetItemCount(ItemID);
}

FPP_EquippedItem UPP_InventoryComponent::QueryEquippedItemInSlot_Implementation(EPP_EquipSlot Slot)
{
	return GetEquippedItemInSlot(Slot);
}

// ===== IPP_InventoryActions 接口实现（转发到 Server RPC） =====

void UPP_InventoryComponent::RequestEquipItem_Implementation(FName ItemID)
{
	// 写路径已统一：武器/护甲都走 Server_EquipItem → EquipItemInternal
	Server_EquipItem(ItemID);
}

void UPP_InventoryComponent::RequestUnequipItem_Implementation(EPP_EquipSlot Slot)
{
	Server_UnequipItem(Slot);
}

void UPP_InventoryComponent::RequestUseConsumable_Implementation(FName ItemID)
{
	Server_UseConsumable(ItemID);
}

// ===== EventBus 广播辅助 =====

void UPP_InventoryComponent::BroadcastInventoryUpdatedEvent()
{
	FPP_InventoryUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	UPP_EventBus::BroadcastEvent(this, Event);
}

void UPP_InventoryComponent::BroadcastEquipmentUpdatedEvent()
{
	FPP_EquipmentUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	UPP_EventBus::BroadcastEvent(this, Event);
}

void UPP_InventoryComponent::BroadcastGoldUpdatedEvent(int32 NewGold)
{
	FPP_GoldUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	Event.NewGold = NewGold;
	UPP_EventBus::BroadcastEvent(this, Event);
}
