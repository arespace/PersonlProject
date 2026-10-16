// 库存系统UI — 背包主界面
// 包含背包格子区域（PanelWidget）和装备栏区域（主手/副手/头/胸/腿/脚/手/戒指）
// 监听库存组件事件自动重绘

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "Framework/PP_EventTypes.h"
#include "Data/PP_ItemTypes.h"
#include "PP_InventoryWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UPP_ItemSlotWidget;
class UPP_EquipSlotWidget;

/**
 * 背包主界面
 *
 * 功能：
 *   - 渲染背包列表（每条 FPP_ItemInstance 对应一个 ItemSlotWidget）
 *   - 渲染装备栏（9 个装备槽位：主手/副手/头部/胸部/腿部/脚部/手部/戒指）
 *   - 显示当前金币数
 *   - 自动监听 InventoryComponent 的事件刷新 UI
 *
 * 蓝图集成：
 *   - 蓝图子类需绑定可选控件：ItemsContainer / GoldText
 *   - 装备槽位控件：MainHandSlot / OffHandSlot / HeadSlot / ChestSlot
 *     / LegsSlot / FeetSlot / HandsSlot / RingSlot
 *   - 设置 ItemSlotWidgetClass 为派生的物品格子蓝图类
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECTUI_API UPP_InventoryWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_InventoryWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 绑定到指定的库存数据源
	 * 调用后会立即刷新一次UI并订阅 EventBus 的库存变更事件
	 * @param InInventory 实现 IPP_InventoryQuery 的对象
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void BindInventory(UObject* InInventory);

	/** 强制刷新整个面板（背包+装备+金币） */
	UFUNCTION(BlueprintCallable, Category = "Inventory|UI")
	void RefreshAll();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** 重建背包格子列表 */
	void RefreshItems();

	/** 重建装备栏 */
	void RefreshEquipment();

	/** 刷新金币文本 */
	void RefreshGold();

	// 事件监听回调（EventBus 订阅）
	void HandleInventoryUpdated(const FPP_InventoryUpdatedEvent& Event);
	void HandleEquipmentUpdated(const FPP_EquipmentUpdatedEvent& Event);
	void HandleGoldUpdated(const FPP_GoldUpdatedEvent& Event);

	/** 工具函数：刷新一个具体的装备槽控件 */
	void RefreshOneEquipSlot(UPP_EquipSlotWidget* SlotWidget, EPP_EquipSlot InSlot);

protected:
	/** 物品格子的容器（建议使用 UWrapBox/UUniformGridPanel/UVerticalBox 等具体子类） */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ItemsContainer;

	// --- 武器槽位 ---
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> MainHandSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> OffHandSlot;

	// --- 防具槽位 ---
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> HeadSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> ChestSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> LegsSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> FeetSlot;

	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> HandsSlot;

	// --- 饰品槽位 ---
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI|Equipment", meta = (BindWidgetOptional))
	TObjectPtr<UPP_EquipSlotWidget> RingSlot;

	/** 金币显示 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI", meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	/** 蓝图配置：用于实例化背包条目的 Widget 子类 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|UI")
	TSubclassOf<UPP_ItemSlotWidget> ItemSlotWidgetClass;

	/** 当前绑定的库存数据源 */
	UPROPERTY(BlueprintReadOnly, Category = "Inventory|UI")
	TWeakObjectPtr<UObject> BoundInventory;

private:
	/** ItemID → Widget 映射，支持增量更新（避免 ClearChildren 全量重建） */
	UPROPERTY()
	TMap<FName, TObjectPtr<UPP_ItemSlotWidget>> ItemWidgetMap;
};
