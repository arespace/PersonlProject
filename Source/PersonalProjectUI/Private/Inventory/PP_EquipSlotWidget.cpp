// 库存系统UI — 装备栏槽位控件实现

#include "Inventory/PP_EquipSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryActions.h"
#include "Inventory/PP_InventoryQuery.h"
#include "Inventory/PP_ItemTooltipWidget.h"

void UPP_EquipSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UPP_EquipSlotWidget::HandleSlotClicked);
	}
}

void UPP_EquipSlotWidget::SetupEquipSlot(UObject* InOwnerInventory,
                                         EPP_EquipSlot InSlot,
                                         const FPP_EquippedItem& InEquippedItem,
                                         const FPP_ItemInfo& InItemInfo)
{
	OwnerInventory = InOwnerInventory;
	SlotType = InSlot;
	CachedEquipped = InEquippedItem;
	CachedInfo = InItemInfo;
	bIsEmptySlot = InEquippedItem.ItemID.IsNone();

	// 蓝图可扩展显示逻辑（如动画/特效）
	OnEquipSlotUpdated(SlotType, CachedEquipped, CachedInfo, bIsEmptySlot);

	// C++ 兜底：确保关键控件一定有值，不被蓝图误清
	if (bIsEmptySlot)
	{
		// 空槽 → 显示默认图标和槽位名称
		if (EquipIcon && !DefaultIcon.IsNull())
		{
			EquipIcon->SetBrushFromTexture(DefaultIcon.LoadSynchronous(), true);
		}
		if (SlotLabel)
		{
			SlotLabel->SetText(DefaultLabel);
		}
	}
	else
	{
		// 有装备 → 显示装备图标和名称
		if (EquipIcon && !InItemInfo.Icon.IsNull())
		{
			EquipIcon->SetBrushFromTexture(InItemInfo.Icon.LoadSynchronous(), true);
		}
		if (SlotLabel)
		{
			SlotLabel->SetText(InItemInfo.DisplayName);
		}
	}
}

void UPP_EquipSlotWidget::HandleSlotClicked()
{
	UObject* Source = OwnerInventory.Get();
	if (!Source || bIsEmptySlot || SlotType == EPP_EquipSlot::None)
	{
		return;
	}
	if (Source->Implements<UPP_InventoryActions>())
	{
		IPP_InventoryActions::Execute_RequestUnequipItem(Source, SlotType);
	}
}

bool UPP_EquipSlotWidget::CanShowTooltip() const
{
	return OwnerInventory.IsValid() && !bIsEmptySlot;
}

bool UPP_EquipSlotWidget::FillTooltipData(UPP_ItemTooltipWidget& Tooltip)
{
	const int32 Count = IPP_InventoryQuery::Execute_QueryItemCount(OwnerInventory.Get(), CachedInfo.ItemID);
	Tooltip.SetupTooltip(OwnerInventory.Get(), CachedInfo, Count, CachedInfo.MaxStackCount);
	return true;
}
