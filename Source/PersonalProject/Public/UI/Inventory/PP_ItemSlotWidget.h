// 库存系统UI — 物品格子控件
// 显示单个物品堆叠（图标+数量），点击触发穿戴或使用

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Inventory/Data/PP_ItemData.h"
#include "Input/Reply.h"
#include "PP_ItemSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;
class UPP_InventoryComponent;

/**
 * 物品格子控件
 * 用于背包面板中显示一条物品实例。
 * 蓝图子类需绑定可选控件：ItemIcon / CountText / SlotButton
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECT_API UPP_ItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 由背包面板调用，初始化此格子
	 * @param InOwnerInventory 所属库存组件
	 * @param InItemInstance   要显示的物品实例
	 * @param InItemInfo       物品配置（用于读取图标/类型/槽位等）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void SetupSlot(UPP_InventoryComponent* InOwnerInventory, const FPP_ItemInstance& InItemInstance, const FPP_ItemInfo& InItemInfo);

	/** 设置提示框 Widget 类（蓝图配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<class UPP_ItemTooltipWidget> TooltipWidgetClass;

protected:
	virtual void NativeConstruct() override;

	/** 点击事件：装备物品请求穿戴；其他物品广播使用事件（暂无实现） */
	UFUNCTION()
	void HandleSlotClicked();

	/** 蓝图可扩展的展示刷新（图标、文字等） */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnSlotUpdated(const FPP_ItemInstance& Instance, const FPP_ItemInfo& Info);

	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

protected:
	/** 物品图标 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	/** 物品名称文本 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	/** 数量文本 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CountText;

	/** 整个格子的可点击按钮 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	/** 当前缓存的库存组件（弱引用即可，避免循环持有） */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	TWeakObjectPtr<UPP_InventoryComponent> OwnerInventory;

	/** 当前缓存的物品实例数据 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	FPP_ItemInstance CachedInstance;

	/** 当前缓存的物品配置数据 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	FPP_ItemInfo CachedInfo;

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
