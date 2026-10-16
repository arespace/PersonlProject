// 库存系统UI — 背包主界面实现

#include "Inventory/PP_InventoryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryQuery.h"
#include "Inventory/PP_ItemSlotWidget.h"
#include "Inventory/PP_EquipSlotWidget.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_DataManager.h"
#include "Engine/GameInstance.h"

UPP_InventoryWidget::UPP_InventoryWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Overlay;
	bConsumesAllInput = true;
}

void UPP_InventoryWidget::NativeConstruct()
{
	Super::NativeConstruct();
}

void UPP_InventoryWidget::NativeDestruct()
{
	// 解除 EventBus 订阅，避免悬挂回调
	if (UPP_EventBus* EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPP_EventBus>() : nullptr)
	{
		EventBus->Unsubscribe(InventoryUpdatedHandle);
		EventBus->Unsubscribe(EquipmentUpdatedHandle);
		EventBus->Unsubscribe(GoldUpdatedHandle);
	}
	InventoryUpdatedHandle.Reset();
	EquipmentUpdatedHandle.Reset();
	GoldUpdatedHandle.Reset();

	ItemWidgetMap.Empty();
	Super::NativeDestruct();
}

void UPP_InventoryWidget::BindInventory(UObject* InInventory)
{
	UPP_EventBus* EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPP_EventBus>() : nullptr;

	// 解绑旧的订阅
	if (EventBus)
	{
		EventBus->Unsubscribe(InventoryUpdatedHandle);
		EventBus->Unsubscribe(EquipmentUpdatedHandle);
		EventBus->Unsubscribe(GoldUpdatedHandle);
	}
	InventoryUpdatedHandle.Reset();
	EquipmentUpdatedHandle.Reset();
	GoldUpdatedHandle.Reset();

	// 清空旧映射 + 容器子控件
	ItemWidgetMap.Empty();
	if (ItemsContainer)
	{
		ItemsContainer->ClearChildren();
	}

	BoundInventory = InInventory;

	if (InInventory && InInventory->Implements<UPP_InventoryQuery>() && EventBus)
	{
		InventoryUpdatedHandle = EventBus->Subscribe<FPP_InventoryUpdatedEvent>(
			[this](const FPP_InventoryUpdatedEvent& Event) { HandleInventoryUpdated(Event); });
		EquipmentUpdatedHandle = EventBus->Subscribe<FPP_EquipmentUpdatedEvent>(
			[this](const FPP_EquipmentUpdatedEvent& Event) { HandleEquipmentUpdated(Event); });
		GoldUpdatedHandle = EventBus->Subscribe<FPP_GoldUpdatedEvent>(
			[this](const FPP_GoldUpdatedEvent& Event) { HandleGoldUpdated(Event); });
	}

	RefreshAll();
}

void UPP_InventoryWidget::RefreshAll()
{
	RefreshItems();
	RefreshEquipment();
	RefreshGold();
}

void UPP_InventoryWidget::RefreshItems()
{
	UObject* Source = BoundInventory.Get();
	if (!ItemsContainer || !Source || !Source->Implements<UPP_InventoryQuery>() || !ItemSlotWidgetClass)
	{
		return;
	}

	const TArray<FPP_ItemInstance> Snapshot = IPP_InventoryQuery::Execute_QueryItems(Source);

	// 收集当前物品ID集合
	TSet<FName> CurrentIDs;
	for (const FPP_ItemInstance& Inst : Snapshot)
	{
		CurrentIDs.Add(Inst.ItemID);
	}

	// 移除已不存在的物品Widget
	TArray<FName> RemovedIDs;
	for (const auto& Pair : ItemWidgetMap)
	{
		if (!CurrentIDs.Contains(Pair.Key))
		{
			if (Pair.Value)
			{
				Pair.Value->RemoveFromParent();
			}
			RemovedIDs.Add(Pair.Key);
		}
	}
	for (const FName& ID : RemovedIDs)
	{
		ItemWidgetMap.Remove(ID);
	}

	// 更新已有Widget或创建新Widget
	for (const FPP_ItemInstance& Inst : Snapshot)
	{
		UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
		const FPP_ItemInfo* Info = DataManager ? DataManager->GetItemInfo(Inst.ItemID) : nullptr;
		if (!Info)
		{
			continue;
		}

		if (TObjectPtr<UPP_ItemSlotWidget>* Existing = ItemWidgetMap.Find(Inst.ItemID))
		{
			// 增量更新已有Widget数据
			if (*Existing)
			{
				(*Existing)->SetupSlot(Source, Inst, *Info);
			}
		}
		else
		{
			// 创建新Widget
			UPP_ItemSlotWidget* SlotWidget = CreateWidget<UPP_ItemSlotWidget>(this, ItemSlotWidgetClass);
			if (!SlotWidget)
			{
				continue;
			}
			SlotWidget->SetupSlot(Source, Inst, *Info);
			ItemsContainer->AddChild(SlotWidget);
			ItemWidgetMap.Add(Inst.ItemID, SlotWidget);
		}
	}
}

void UPP_InventoryWidget::RefreshEquipment()
{
	if (!BoundInventory.IsValid()) return;

	// 武器槽
	RefreshOneEquipSlot(MainHandSlot,  EPP_EquipSlot::MainHand);
	RefreshOneEquipSlot(OffHandSlot,   EPP_EquipSlot::OffHand);

	// 防具槽
	RefreshOneEquipSlot(HeadSlot,      EPP_EquipSlot::Head);
	RefreshOneEquipSlot(ChestSlot,     EPP_EquipSlot::Chest);
	RefreshOneEquipSlot(LegsSlot,      EPP_EquipSlot::Legs);
	RefreshOneEquipSlot(FeetSlot,      EPP_EquipSlot::Feet);
	RefreshOneEquipSlot(HandsSlot,     EPP_EquipSlot::Hands);

	// 饰品槽
	RefreshOneEquipSlot(RingSlot,      EPP_EquipSlot::Ring);
}

void UPP_InventoryWidget::RefreshOneEquipSlot(UPP_EquipSlotWidget* SlotWidget, EPP_EquipSlot InSlot)
{
	UObject* Source = BoundInventory.Get();
	if (!SlotWidget || !Source || !Source->Implements<UPP_InventoryQuery>()) return;

	FPP_EquippedItem Found;
	FPP_ItemInfo InfoCopy;

	for (const FPP_EquippedItem& E : IPP_InventoryQuery::Execute_QueryEquippedItems(Source))
	{
		if (E.Slot == InSlot)
		{
			Found = E;
			UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
			if (const FPP_ItemInfo* Info = DataManager ? DataManager->GetItemInfo(E.ItemID) : nullptr)
			{
				InfoCopy = *Info;
			}
			break;
		}
	}

	SlotWidget->SetupEquipSlot(Source, InSlot, Found, InfoCopy);
}

void UPP_InventoryWidget::RefreshGold()
{
	UObject* Source = BoundInventory.Get();
	if (!GoldText || !Source || !Source->Implements<UPP_InventoryQuery>()) return;
	GoldText->SetText(FText::AsNumber(IPP_InventoryQuery::Execute_QueryGold(Source)));
}

// ===== 事件回调：仅做局部刷新 =====

void UPP_InventoryWidget::HandleInventoryUpdated(const FPP_InventoryUpdatedEvent& Event)
{
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}
	RefreshItems();
}

void UPP_InventoryWidget::HandleEquipmentUpdated(const FPP_EquipmentUpdatedEvent& Event)
{
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}
	RefreshEquipment();
}

void UPP_InventoryWidget::HandleGoldUpdated(const FPP_GoldUpdatedEvent& Event)
{
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}
	RefreshGold();
}
