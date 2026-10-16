// 库存系统UI — 槽位控件 Tooltip 生命周期基类实现

#include "Inventory/PP_SlotWidgetBase.h"

#include "Inventory/PP_ItemTooltipWidget.h"
#include "Framework/PP_UIManager.h"
#include "TimerManager.h"

void UPP_SlotWidgetBase::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);

	if (!TooltipWidgetClass || !CanShowTooltip())
	{
		return;
	}

	// 0.3秒延迟后显示提示框
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			TooltipTimerHandle,
			this,
			&UPP_SlotWidgetBase::ShowTooltip,
			0.3f,
			false);
	}
}

void UPP_SlotWidgetBase::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);

	// 取消延迟计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(TooltipTimerHandle);
	}

	HideTooltip();
}

FReply UPP_SlotWidgetBase::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// 更新提示框位置
	if (ActiveTooltip)
	{
		ActiveTooltip->UpdatePosition(InMouseEvent.GetScreenSpacePosition());
	}
	return Super::NativeOnMouseMove(InGeometry, InMouseEvent);
}

void UPP_SlotWidgetBase::ShowTooltip()
{
	if (ActiveTooltip || !CanShowTooltip())
	{
		return;
	}

	ActiveTooltip = CreateWidget<UPP_ItemTooltipWidget>(this, TooltipWidgetClass);
	if (!ActiveTooltip)
	{
		return;
	}

	// 数据填充失败则丢弃刚创建的提示框，不显示
	if (!FillTooltipData(*ActiveTooltip))
	{
		ActiveTooltip = nullptr;
		return;
	}

	ActiveTooltip->AddToViewport(static_cast<int32>(EPP_UILayer::Overlay));
}

void UPP_SlotWidgetBase::HideTooltip()
{
	if (ActiveTooltip)
	{
		// 通过 UIManager 弹出，避免直接 RemoveFromParent 导致 NativeDestruct 中重复弹出
		if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
		{
			Mgr->PopWidget(ActiveTooltip);
		}
		ActiveTooltip = nullptr;
	}
}
