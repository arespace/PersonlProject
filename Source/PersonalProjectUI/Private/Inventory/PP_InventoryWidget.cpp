// 库存系统UI — 背包主界面实现

#include "Inventory/PP_InventoryWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Inventory/PP_ItemSlotWidget.h"
#include "Inventory/PP_EquipSlotWidget.h"

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
	// 解除事件订阅，避免悬挂回调
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UPP_InventoryWidget::HandleInventoryChanged);
		BoundInventory->OnEquipmentChanged.RemoveDynamic(this, &UPP_InventoryWidget::HandleEquipmentChanged);
		BoundInventory->OnGoldChanged.RemoveDynamic(this, &UPP_InventoryWidget::HandleGoldChanged);
	}

	ItemWidgetMap.Empty();
	Super::NativeDestruct();
}

void UPP_InventoryWidget::BindInventory(UPP_InventoryComponent* InInventory)
{
	// 解绑旧的
	if (BoundInventory.IsValid())
	{
		BoundInventory->OnInventoryChanged.RemoveDynamic(this, &UPP_InventoryWidget::HandleInventoryChanged);
		BoundInventory->OnEquipmentChanged.RemoveDynamic(this, &UPP_InventoryWidget::HandleEquipmentChanged);
		BoundInventory->OnGoldChanged.RemoveDynamic(this, &UPP_InventoryWidget::HandleGoldChanged);
	}

	// 清空旧映射 + 容器子控件
	ItemWidgetMap.Empty();
	if (ItemsContainer)
	{
		ItemsContainer->ClearChildren();
	}

	BoundInventory = InInventory;

	if (InInventory)
	{
		InInventory->OnInventoryChanged.AddDynamic(this, &UPP_InventoryWidget::HandleInventoryChanged);
		InInventory->OnEquipmentChanged.AddDynamic(this, &UPP_InventoryWidget::HandleEquipmentChanged);
		InInventory->OnGoldChanged.AddDynamic(this, &UPP_InventoryWidget::HandleGoldChanged);
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
	if (!ItemsContainer || !BoundInventory.IsValid() || !ItemSlotWidgetClass)
	{
		return;
	}

	const TArray<FPP_ItemInstance> Snapshot = BoundInventory->GetItems();

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
		const FPP_ItemInfo* Info = BoundInventory->FindItemInfo(Inst.ItemID);
		if (!Info)
		{
			continue;
		}

		if (TObjectPtr<UPP_ItemSlotWidget>* Existing = ItemWidgetMap.Find(Inst.ItemID))
		{
			// 增量更新已有Widget数据
			if (*Existing)
			{
				(*Existing)->SetupSlot(BoundInventory.Get(), Inst, *Info);
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
			SlotWidget->SetupSlot(BoundInventory.Get(), Inst, *Info);
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
	if (!SlotWidget || !BoundInventory.IsValid()) return;

	FPP_EquippedItem Found;
	FPP_ItemInfo InfoCopy;

	for (const FPP_EquippedItem& E : BoundInventory->GetEquippedItems())
	{
		if (E.Slot == InSlot)
		{
			Found = E;
			if (const FPP_ItemInfo* Info = BoundInventory->FindItemInfo(E.ItemID))
			{
				InfoCopy = *Info;
			}
			break;
		}
	}

	SlotWidget->SetupEquipSlot(BoundInventory.Get(), InSlot, Found, InfoCopy);
}

void UPP_InventoryWidget::RefreshGold()
{
	if (!GoldText || !BoundInventory.IsValid()) return;
	GoldText->SetText(FText::AsNumber(BoundInventory->GetGold()));
}

// ===== 事件回调：仅做局部刷新 =====

void UPP_InventoryWidget::HandleInventoryChanged(FName /*ItemID*/, int32 /*NewCount*/)
{
	RefreshItems();
}

void UPP_InventoryWidget::HandleEquipmentChanged(EPP_EquipSlot /*InSlot*/, FName /*ItemID*/)
{
	RefreshEquipment();
}

void UPP_InventoryWidget::HandleGoldChanged(int32 /*NewGold*/)
{
	RefreshGold();
}
