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
#include "Interfaces/PP_InteractableInterface.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"

namespace
{
	/** 旧装备槽 → 武器槽的对应关系（仅武器两槽） */
	bool ToWeaponSlotType(EPP_EquipSlot Slot, EPP_EquipmentSlotType& OutSlotType)
	{
		switch (Slot)
		{
		case EPP_EquipSlot::MainHand: OutSlotType = EPP_EquipmentSlotType::PrimaryWeapon;   return true;
		case EPP_EquipSlot::OffHand:  OutSlotType = EPP_EquipmentSlotType::SecondaryWeapon; return true;
		default: return false;
		}
	}

	/** 武器槽 → 旧装备槽的对应关系（仅武器两槽） */
	bool ToEquipSlot(EPP_EquipmentSlotType SlotType, EPP_EquipSlot& OutSlot)
	{
		switch (SlotType)
		{
		case EPP_EquipmentSlotType::PrimaryWeapon:   OutSlot = EPP_EquipSlot::MainHand; return true;
		case EPP_EquipmentSlotType::SecondaryWeapon: OutSlot = EPP_EquipSlot::OffHand;  return true;
		default: return false;
		}
	}
}

UPP_InventoryComponent::UPP_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Gold = 1000;

	// 武器系统 4 个固定装备槽
	EquipmentSlots.Emplace(EPP_EquipmentSlotType::PrimaryWeapon);
	EquipmentSlots.Emplace(EPP_EquipmentSlotType::SecondaryWeapon);
	EquipmentSlots.Emplace(EPP_EquipmentSlotType::Helmet);
	EquipmentSlots.Emplace(EPP_EquipmentSlotType::Accessory);
}

void UPP_InventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UPP_InventoryComponent, Items);
	DOREPLIFETIME(UPP_InventoryComponent, EquippedItems);
	DOREPLIFETIME(UPP_InventoryComponent, EquipmentSlots);
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
	if (!HasItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[Inventory] EquipItem: 背包中无物品 %s"), *ItemID.ToString());
		return;
	}
	const FPP_ItemInfo* Info = FindItemInfo(ItemID);
	if (!Info)
	{
		return;
	}
	if (Info->ItemType != EPP_ItemType::Equipment || Info->EquipSlot == EPP_EquipSlot::None)
	{
		UE_LOG(LogPP, Warning, TEXT("[Inventory] EquipItem: 物品 %s 不是装备类型"), *ItemID.ToString());
		return;
	}

	// 1) 若该槽位已有装备，先脱下并将旧物品归还背包
	if (GetEquippedItemID(Info->EquipSlot) != NAME_None)
	{
		const FName OldItemID = ApplyUnequip(Info->EquipSlot);
		if (!OldItemID.IsNone())
		{
			AddItem(OldItemID, 1);
		}
	}

	// 2) 从背包扣减一件
	if (!RemoveItem(ItemID, 1))
	{
		return;
	}

	// 3) 施加 GE 并记录句柄
	ApplyEquip(*Info);
}

void UPP_InventoryComponent::Server_UnequipItem_Implementation(EPP_EquipSlot Slot)
{
	if (Slot == EPP_EquipSlot::None) return;

	const FName ItemID = ApplyUnequip(Slot);
	if (!ItemID.IsNone())
	{
		AddItem(ItemID, 1);
	}
}

void UPP_InventoryComponent::ApplyEquip(const FPP_ItemInfo& ItemInfo)
{
	FPP_EquippedItem NewEquip;
	NewEquip.ItemID = ItemInfo.ItemID;
	NewEquip.Slot = ItemInfo.EquipSlot;

	// 通过拥有者 ASC 施加 GE
	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		if (ItemInfo.EquipEffectClass)
		{
			FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
			Context.AddSourceObject(GetOwner());
			const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemInfo.EquipEffectClass, 1.0f, Context);
			if (SpecHandle.IsValid())
			{
				NewEquip.EffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	} 

	EquippedItems.Add(NewEquip);
	BroadcastEquipmentUpdatedEvent();
}

FName UPP_InventoryComponent::ApplyUnequip(EPP_EquipSlot Slot)
{
	const int32 Idx = EquippedItems.IndexOfByPredicate([Slot](const FPP_EquippedItem& E)
	{
		return E.Slot == Slot;
	});
	if (Idx == INDEX_NONE)
	{
		return NAME_None;
	}

	const FPP_EquippedItem Old = EquippedItems[Idx];

	// 通过句柄精确移除 GE
	if (Old.EffectHandle.IsValid())
	{
		if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
		{
			ASC->RemoveActiveGameplayEffect(Old.EffectHandle);
		}
	}

	EquippedItems.RemoveAt(Idx);
	BroadcastEquipmentUpdatedEvent();
	return Old.ItemID;
}

FName UPP_InventoryComponent::GetEquippedItemID(EPP_EquipSlot Slot) const
{
	return GetEquippedItemInSlot(Slot).ItemID;
}

void UPP_InventoryComponent::ApplyWeaponEffect(EPP_EquipmentSlotType SlotType, FName ItemID)
{
	// 换装时先移除该槽位上一把武器的属性效果，避免叠加
	RemoveWeaponEffect(SlotType);

	const FPP_ItemInfo* ItemInfo = FindItemInfo(ItemID);
	if (!ItemInfo || !ItemInfo->EquipEffectClass)
	{
		// 武器未配置属性效果（纯技能武器）属正常情况
		return;
	}

	UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 槽 %d 装备 %s：ASC 无效，属性效果未施加"),
			static_cast<int32>(SlotType), *ItemID.ToString());
		return;
	}

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(GetOwner());
	const FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(ItemInfo->EquipEffectClass, 1.0f, Context);
	if (!SpecHandle.IsValid())
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 槽 %d 装备 %s：EquipEffectClass 无法生成 Spec"),
			static_cast<int32>(SlotType), *ItemID.ToString());
		return;
	}

	const FActiveGameplayEffectHandle Handle = ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	if (Handle.IsValid())
	{
		WeaponEffectHandles.Add(SlotType, Handle);
		UE_LOG(LogPP, Log, TEXT("[WeaponEquip] 槽 %d 装备 %s：属性效果已施加"),
			static_cast<int32>(SlotType), *ItemID.ToString());
	}
}

void UPP_InventoryComponent::RemoveWeaponEffect(EPP_EquipmentSlotType SlotType)
{
	FActiveGameplayEffectHandle Handle;
	if (!WeaponEffectHandles.RemoveAndCopyValue(SlotType, Handle) || !Handle.IsValid())
	{
		return;
	}

	if (UAbilitySystemComponent* ASC = GetOwningAbilitySystemComponent())
	{
		ASC->RemoveActiveGameplayEffect(Handle);
	}
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
	// 武器两槽以 EquipmentSlots 为唯一数据源（新装备链路只写这里）
	EPP_EquipmentSlotType WeaponSlotType;
	if (ToWeaponSlotType(Slot, WeaponSlotType))
	{
		for (const FPP_EquipmentSlot& S : EquipmentSlots)
		{
			if (S.SlotType != WeaponSlotType)
			{
				continue;
			}
			if (!S.ItemID.IsNone())
			{
				FPP_EquippedItem Equipped;
				Equipped.ItemID = S.ItemID;
				Equipped.Slot = Slot;
				return Equipped;
			}
			break;
		}
		return FPP_EquippedItem();
	}

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
	// 武器两槽由 EquipmentSlots 提供（新链路只写这里），其余槽位仍来自旧轨 EquippedItems
	TArray<FPP_EquippedItem> Result;
	Result.Reserve(EquippedItems.Num() + EquipmentSlots.Num());

	for (const FPP_EquippedItem& E : EquippedItems)
	{
		if (E.Slot != EPP_EquipSlot::MainHand && E.Slot != EPP_EquipSlot::OffHand)
		{
			Result.Add(E);
		}
	}

	for (const FPP_EquipmentSlot& S : EquipmentSlots)
	{
		EPP_EquipSlot EquipSlot;
		if (S.ItemID.IsNone() || !ToEquipSlot(S.SlotType, EquipSlot))
		{
			continue;
		}
		FPP_EquippedItem Equipped;
		Equipped.ItemID = S.ItemID;
		Equipped.Slot = EquipSlot;
		Result.Add(Equipped);
	}

	return Result;
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
	// 主手/副手武器走新的装备槽系统；防具等仍走旧轨（旧轨统一留到装备槽批次再删）
	if (const FPP_ItemInfo* Info = FindItemInfo(ItemID))
	{
		if (Info->ItemType == EPP_ItemType::Equipment && Info->EquipSlot == EPP_EquipSlot::MainHand)
		{
			Server_EquipWeapon(ItemID, EPP_EquipmentSlotType::PrimaryWeapon);
			return;
		}
		if (Info->ItemType == EPP_ItemType::Equipment && Info->EquipSlot == EPP_EquipSlot::OffHand)
		{
			Server_EquipWeapon(ItemID, EPP_EquipmentSlotType::SecondaryWeapon);
			return;
		}
	}

	Server_EquipItem(ItemID);
}

void UPP_InventoryComponent::RequestUnequipItem_Implementation(EPP_EquipSlot Slot)
{
	if (Slot == EPP_EquipSlot::MainHand)
	{
		Server_UnequipWeapon(EPP_EquipmentSlotType::PrimaryWeapon);
		return;
	}
	if (Slot == EPP_EquipSlot::OffHand)
	{
		Server_UnequipWeapon(EPP_EquipmentSlotType::SecondaryWeapon);
		return;
	}

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

// ===== 武器系统装备槽实现（EPP_EquipmentSlotType） =====

void UPP_InventoryComponent::Server_EquipWeapon_Implementation(FName ItemID, EPP_EquipmentSlotType SlotType)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (ItemID.IsNone())
	{
		return;
	}

	// 找到对应槽位（构造函数已初始化 4 槽）
	const int32 Idx = EquipmentSlots.IndexOfByPredicate([SlotType](const FPP_EquipmentSlot& S)
	{
		return S.SlotType == SlotType;
	});
	if (Idx == INDEX_NONE)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 装备槽 %d 不存在（EquipmentSlots 未初始化？共 %d 个）"),
			static_cast<int32>(SlotType), EquipmentSlots.Num());
		return;
	}

	// 记录旧值用于回调
	const FName OldItemID = EquipmentSlots[Idx].ItemID;
	if (OldItemID == ItemID)
	{
		return; // 相同物品无需重复装备
	}

	// 新武器必须先在背包里（防止凭空装备）
	if (!HasItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 背包中没有 %s，装备失败"), *ItemID.ToString());
		return;
	}

	// 新武器出背包
	if (!RemoveItem(ItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 扣减背包 %s 失败"), *ItemID.ToString());
		return;
	}

	// 旧武器回背包（保证换下的武器还能再装回去）
	if (!OldItemID.IsNone() && !AddItem(OldItemID, 1))
	{
		// 背包已满：把新武器还回背包，取消换装，避免物品凭空消失
		AddItem(ItemID, 1);
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 背包已满，无法收回旧武器 %s，换装取消"), *OldItemID.ToString());
		return;
	}

	// 更新槽位
	EquipmentSlots[Idx].ItemID = ItemID;

	// 施加武器属性效果（旧效果在 ApplyWeaponEffect 内部移除）
	ApplyWeaponEffect(SlotType, ItemID);

	// 通知 UI 刷新装备栏（服务端不会触发 OnRep_WeaponSlots）
	BroadcastEquipmentUpdatedEvent();

	// 通知拥有者 Pawn（PlayerCharacter 实现 IPP_EquipmentUser）
	// BlueprintNativeEvent 不能直接调用，必须走 Execute_ 包装
	AActor* NotifyTarget = GetEquipmentUserActor();
	if (NotifyTarget && NotifyTarget->GetClass()->ImplementsInterface(UPP_EquipmentUser::StaticClass()))
	{
		IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, SlotType, ItemID);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 未找到装备使用者 Actor（Pawn 为空或未实现 IPP_EquipmentUser），外观/技能不会切换"));
	}
}

void UPP_InventoryComponent::Server_UnequipWeapon_Implementation(EPP_EquipmentSlotType SlotType)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	const int32 Idx = EquipmentSlots.IndexOfByPredicate([SlotType](const FPP_EquipmentSlot& S)
	{
		return S.SlotType == SlotType;
	});
	if (Idx == INDEX_NONE)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] 卸下失败：槽位 %d 不存在"), static_cast<int32>(SlotType));
		return;
	}

	const FName OldItemID = EquipmentSlots[Idx].ItemID;
	if (OldItemID.IsNone())
	{
		return;
	}

	// 卸下的武器回到背包；背包满则取消卸下，避免物品凭空消失
	if (!AddItem(OldItemID, 1))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponEquip] UnequipWeapon: 背包已满，无法收回 %s，卸下取消"), *OldItemID.ToString());
		return;
	}

	EquipmentSlots[Idx].ItemID = NAME_None;

	// 移除武器属性效果
	RemoveWeaponEffect(SlotType);

	// 通知 UI 刷新装备栏（服务端不会触发 OnRep_WeaponSlots）
	BroadcastEquipmentUpdatedEvent();

	// 通知装备用户（BlueprintNativeEvent 必须走 Execute_ 包装）
	if (AActor* NotifyTarget = GetEquipmentUserActor())
	{
		if (NotifyTarget->GetClass()->ImplementsInterface(UPP_EquipmentUser::StaticClass()))
		{
			IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, SlotType, NAME_None);
		}
	}
}

FName UPP_InventoryComponent::GetWeaponInSlot(EPP_EquipmentSlotType SlotType) const
{
	for (const FPP_EquipmentSlot& S : EquipmentSlots)
	{
		if (S.SlotType == SlotType)
		{
			return S.ItemID;
		}
	}
	return NAME_None;
}

void UPP_InventoryComponent::OnRep_WeaponSlots(const TArray<FPP_EquipmentSlot>& OldSlots)
{
	// UI 装备栏刷新不依赖 Pawn 是否就绪，先广播事件
	BroadcastEquipmentUpdatedEvent();

	// 客户端收到武器槽复制：SetMaterial / SetStaticMesh 不参与网络复制，
	// 因此外观（隐藏自带武器 / 显示装备武器网格）必须在客户端收到槽位变化后重新应用一次。
	AActor* NotifyTarget = GetEquipmentUserActor();
	if (!IsValid(NotifyTarget) || !NotifyTarget->GetClass()->ImplementsInterface(UPP_EquipmentUser::StaticClass()))
	{
		// 客户端 Pawn 可能还没生成，此时外观由角色 BeginPlay 的默认状态负责
		return;
	}

	for (const FPP_EquipmentSlot& Slot : EquipmentSlots)
	{
		const FPP_EquipmentSlot* OldSlot = OldSlots.FindByPredicate(
			[Slot](const FPP_EquipmentSlot& Candidate) { return Candidate.SlotType == Slot.SlotType; });

		const FName OldItemID = OldSlot ? OldSlot->ItemID : NAME_None;
		if (OldItemID == Slot.ItemID)
		{
			continue;
		}

		IPP_EquipmentUser::Execute_OnEquipmentChanged(NotifyTarget, Slot.SlotType, Slot.ItemID);
	}
}
