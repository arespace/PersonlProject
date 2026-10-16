// 库存系统UI — 物品格子控件实现

#include "Inventory/PP_ItemSlotWidget.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryActions.h"
#include "Inventory/PP_ItemTooltipWidget.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_UIManager.h"
#include "Engine/GameInstance.h"
#include "TimerManager.h"

void UPP_ItemSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();
	if (SlotButton)
	{
		SlotButton->OnClicked.RemoveDynamic(this, &UPP_ItemSlotWidget::HandleSlotClicked);
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

void UPP_ItemSlotWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!TooltipWidgetClass || !OwnerInventory.IsValid())
	{
		return;
	}

	// 0.3秒延迟后显示提示框
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			TooltipTimerHandle,
			this,
			&UPP_ItemSlotWidget::ShowTooltip,
			0.3f,
			false);
	}
}

void UPP_ItemSlotWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// 取消延迟计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TooltipTimerHandle);
	}

	HideTooltip();
}

FReply UPP_ItemSlotWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 更新提示框位置
	if (ActiveTooltip)
	{
		ActiveTooltip->UpdatePosition(InMouseEvent.GetScreenSpacePosition());
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UPP_ItemSlotWidget::ShowTooltip()
{
	if (ActiveTooltip || !OwnerInventory.IsValid())
	{
		return;
	}

	ActiveTooltip = CreateWidget<UPP_ItemTooltipWidget>(this, TooltipWidgetClass);
	if (!ActiveTooltip)
	{
		return;
	}

	// 查找物品配置以获取 MaxStackCount（全局物品表走 DataManager）
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	const FPP_ItemInfo* Info = DataManager ? DataManager->GetItemInfo(CachedInstance.ItemID) : nullptr;
	if (!Info)
	{
		return;
	}

	ActiveTooltip->SetupTooltip(OwnerInventory.Get(), *Info, CachedInstance.Count, Info->MaxStackCount);
	ActiveTooltip->AddToViewport(static_cast<int32>(EPP_UILayer::Overlay));
}

void UPP_ItemSlotWidget::HideTooltip()
{
	if (ActiveTooltip)
	{
		// 通过 UIManager 弹出，避免直接 RemoveFromParent 导致 NativeDestruct 中重复弹出
		if (UGameInstance* GI = GetGameInstance())
		{
			if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
			{
				Mgr->PopWidget(ActiveTooltip);
			}
		}
		ActiveTooltip = nullptr;
	}
}
