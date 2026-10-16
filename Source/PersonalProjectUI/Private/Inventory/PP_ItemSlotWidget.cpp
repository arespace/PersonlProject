// 库存系统UI — 物品格子控件实现

#include "Inventory/PP_ItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryActions.h"
#include "Inventory/PP_ItemTooltipWidget.h"
#include "Framework/PP_DataManager.h"

void UPP_ItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SlotButton)
	{
		SlotButton->OnClicked.AddDynamic(this, &UPP_ItemSlotWidget::HandleSlotClicked);
	}
}

void UPP_ItemSlotWidget::SetupSlot(UObject* InOwnerInventory, const FPP_ItemInstance& InItemInstance, const FPP_ItemInfo& InItemInfo)
{
	OwnerInventory = InOwnerInventory;
	CachedInstance = InItemInstance;
	CachedInfo = InItemInfo;

	// 蓝图先执行自定义显示逻辑（如有）
	OnSlotUpdated(CachedInstance, CachedInfo);

	// C++ 兜底填充：确保关键控件一定有值，不被蓝图误清
	if (ItemIcon && !InItemInfo.Icon.IsNull())
	{
		ItemIcon->SetBrushFromTexture(InItemInfo.Icon.LoadSynchronous(), true);
	}
	if (ItemNameText)
	{
		ItemNameText->SetText(InItemInfo.DisplayName);
	}
	if (CountText)
	{
		CountText->SetText(FText::AsNumber(InItemInstance.Count));
	}
}

void UPP_ItemSlotWidget::HandleSlotClicked()
{
	UObject* Source = OwnerInventory.Get();
	if (!Source || !Source->Implements<UPP_InventoryActions>())
	{
		return;
	}

	// 装备类物品：请求穿戴
	if (CachedInfo.ItemType == EPP_ItemType::Equipment && CachedInfo.EquipSlot != EPP_EquipSlot::None)
	{
		IPP_InventoryActions::Execute_RequestEquipItem(Source, CachedInstance.ItemID);
	}
	// 消耗品类物品：请求使用
	else if (CachedInfo.ItemType == EPP_ItemType::Consumable)
	{
		IPP_InventoryActions::Execute_RequestUseConsumable(Source, CachedInstance.ItemID);
	}
}

bool UPP_ItemSlotWidget::CanShowTooltip() const
{
	return OwnerInventory.IsValid();
}

bool UPP_ItemSlotWidget::FillTooltipData(UPP_ItemTooltipWidget& Tooltip)
{
	// 查找物品配置以获取 MaxStackCount（全局物品表走 DataManager）
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	const FPP_ItemInfo* Info = DataManager ? DataManager->GetItemInfo(CachedInstance.ItemID) : nullptr;
	if (!Info)
	{
		return false;
	}

	Tooltip.SetupTooltip(OwnerInventory.Get(), *Info, CachedInstance.Count, Info->MaxStackCount);
	return true;
}
