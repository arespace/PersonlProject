// 库存系统 — 库存与装备管理组件实现
// 关键逻辑：堆叠合并、装备替换、GE施加/移除、网络复制

#include "Inventory/PP_InventoryComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffect.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "Engine/GameInstance.h"
#include "Framework/PP_Log.h"

UPP_InventoryComponent::UPP_InventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
	Gold = 1000;
}

void UPP_InventoryComponent::BeginPlay()
{
	Super::BeginPlay();
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
	OnInventoryChanged.Broadcast(ItemID, NewTotal);
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
	OnInventoryChanged.Broadcast(ItemID, NewTotal);
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
	OnEquipmentChanged.Broadcast(NewEquip.Slot, NewEquip.ItemID);
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
	OnEquipmentChanged.Broadcast(Slot, NAME_None);
	BroadcastEquipmentUpdatedEvent();
	return Old.ItemID;
}

FName UPP_InventoryComponent::GetEquippedItemID(EPP_EquipSlot Slot) const
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
	OnGoldChanged.Broadcast(Gold);
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
	OnGoldChanged.Broadcast(Gold);
	BroadcastGoldUpdatedEvent(Gold);
}

// ===== OnRep 回调 =====

void UPP_InventoryComponent::OnRep_Items(const TArray<FPP_ItemInstance>& OldItems)
{
	// 客户端通用刷新：广播一次空ID事件，UI 监听者据此整体重绘
	OnInventoryChanged.Broadcast(NAME_None, 0);
	BroadcastInventoryUpdatedEvent();
}

void UPP_InventoryComponent::OnRep_Equipped(const TArray<FPP_EquippedItem>& OldEquipped)
{
	// 装备列表变化：触发 UI 重绘
	OnEquipmentChanged.Broadcast(EPP_EquipSlot::None, NAME_None);
	BroadcastEquipmentUpdatedEvent();
}

void UPP_InventoryComponent::OnRep_Gold(int32 OldGold)
{
	OnGoldChanged.Broadcast(Gold);
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
	OnItemUsed.Broadcast(ItemID);

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
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UPP_EventBus* EventBus = GI ? GI->GetSubsystem<UPP_EventBus>() : nullptr;
	if (!EventBus) return;

	FPP_InventoryUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	EventBus->Broadcast(Event);
}

void UPP_InventoryComponent::BroadcastEquipmentUpdatedEvent()
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UPP_EventBus* EventBus = GI ? GI->GetSubsystem<UPP_EventBus>() : nullptr;
	if (!EventBus) return;

	FPP_EquipmentUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	EventBus->Broadcast(Event);
}

void UPP_InventoryComponent::BroadcastGoldUpdatedEvent(int32 NewGold)
{
	UWorld* World = GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	UPP_EventBus* EventBus = GI ? GI->GetSubsystem<UPP_EventBus>() : nullptr;
	if (!EventBus) return;

	FPP_GoldUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	Event.NewGold = NewGold;
	EventBus->Broadcast(Event);
}
