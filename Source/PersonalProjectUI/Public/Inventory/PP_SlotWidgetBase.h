// 库存系统UI — 带物品提示框的槽位控件基类
// 收敛 ItemSlot/EquipSlot 共用的「悬停延迟显示 Tooltip、移动跟随、离开隐藏」生命周期

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Input/Reply.h"
#include "PP_SlotWidgetBase.generated.h"

class UPP_ItemTooltipWidget;

/**
 * UPP_SlotWidgetBase — 槽位类控件的 Tooltip 生命周期基类
 *
 * 使用约定：
 *   - 子类配置 TooltipWidgetClass 后，悬停 0.3 秒自动弹出提示框，移动跟随、离开隐藏
 *   - 子类 override FillTooltipData 填充提示内容
 *   - 不需要提示框的子类不配置 TooltipWidgetClass 即可
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECTUI_API UPP_SlotWidgetBase : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 提示框 Widget 类（蓝图配置；为空则不显示提示框） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UPP_ItemTooltipWidget> TooltipWidgetClass;

protected:
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	/** 是否允许显示提示框（子类可 override，如库存源失效、空槽时返回 false） */
	virtual bool CanShowTooltip() const { return true; }

	/** 向提示框填充内容；返回 false 则取消显示（如数据缺失）。子类必须 override */
	virtual bool FillTooltipData(UPP_ItemTooltipWidget& Tooltip) { return false; }

private:
	/** 当前显示的提示框实例 */
	UPROPERTY()
	TObjectPtr<UPP_ItemTooltipWidget> ActiveTooltip;

	/** 提示框延迟计时器句柄 */
	FTimerHandle TooltipTimerHandle;

	/** 延迟后显示提示框 */
	void ShowTooltip();

	/** 隐藏提示框 */
	void HideTooltip();
};
