// 库存系统UI — 装备栏槽位控件
// 显示单个装备槽位（武器/护甲/饰品），点击触发脱下

#pragma once

#include "CoreMinimal.h"
#include "Data/PP_ItemData.h"
#include "Data/PP_ItemTypes.h"
#include "Inventory/PP_SlotWidgetBase.h"
#include "PP_EquipSlotWidget.generated.h"

class UImage;
class UTextBlock;
class UButton;

/**
 * 装备栏槽位控件
 * 用于装备区域显示某一个固定槽位（武器/护甲/饰品）的当前装备。
 * 蓝图子类需绑定可选控件：EquipIcon / SlotLabel / SlotButton
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECTUI_API UPP_EquipSlotWidget : public UPP_SlotWidgetBase
{
	GENERATED_BODY()

public:
	/**
	 * 初始化装备槽
	 * @param InOwnerInventory 所属库存数据源（实现 IPP_InventoryQuery / IPP_InventoryActions 的对象）
	 * @param InSlot           本槽位代表的装备槽位
	 * @param InEquippedItem   当前装备数据（若为空则显示空槽）
	 * @param InItemInfo       已装备物品的配置（图标等）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void SetupEquipSlot(UObject* InOwnerInventory,
	                    EPP_EquipSlot InSlot,
	                    const FPP_EquippedItem& InEquippedItem,
	                    const FPP_ItemInfo& InItemInfo);

	/** 空槽时的默认图标（蓝图配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	TSoftObjectPtr<UTexture2D> DefaultIcon;

	/** 空槽时的默认文本（蓝图配置，如"武器"/"护甲"/"饰品"） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	FText DefaultLabel;

protected:
	virtual void NativeConstruct() override;

	/** 点击：若当前槽位有装备，则发送脱下RPC */
	UFUNCTION()
	void HandleSlotClicked();

	/** 蓝图可扩展的展示刷新 */
	UFUNCTION(BlueprintImplementableEvent, Category = "Inventory|UI")
	void OnEquipSlotUpdated(EPP_EquipSlot InSlot, const FPP_EquippedItem& Equipped, const FPP_ItemInfo& Info, bool bIsEmpty);

	virtual bool CanShowTooltip() const override;
	virtual bool FillTooltipData(UPP_ItemTooltipWidget& Tooltip) override;

protected:
	/** 装备图标 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UImage> EquipIcon;

	/** 槽位名称文本（武器/护甲/饰品） */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SlotLabel;

	/** 整槽点击按钮 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UButton> SlotButton;

	/** 当前所属库存数据源 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	TWeakObjectPtr<UObject> OwnerInventory;

	/** 此控件代表的槽位（蓝图可在编辑器配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	EPP_EquipSlot SlotType = EPP_EquipSlot::None;

	/** 缓存的当前装备 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	FPP_EquippedItem CachedEquipped;

	/** 缓存的物品配置 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	FPP_ItemInfo CachedInfo;

	/** 当前是否为空槽 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	bool bIsEmptySlot = true;
};
