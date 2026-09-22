// 商店系统 — 单个商品条目UI控件
// 显示物品图标、名称、价格、持有数量；根据启用状态控制可交互性

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_ShopItemWidget.generated.h"

class UTextBlock;
class UImage;
class UButton;
class UTexture2D;
class UBorder;
class UPP_BuyCountWidget;
class UPP_ShopComponent;
class UPP_InventoryComponent;

/** 打开数量选择弹窗委托：商品ID, 名称, 单价, 最大数量, 是否购买模式 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnOpenBuyCountPopup, FName, ItemID, const FText&, ItemName, int32, UnitPrice, int32, MaxCount, bool, bBuyMode);

/**
 * 商品条目 Widget
 *
 * 职责：
 *   1. 显示单个商品的图标、名称、价格、持有数量
 *   2. 提供出售按钮，点击弹出数量选择器
 *   3. 支持禁用状态：不可出售时按钮灰显、背景变白
 */
UCLASS()
class PERSONALPROJECT_API UPP_ShopItemWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 配置商品显示信息 */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void SetupItem(FName InItemID, const FText& InName, int32 InPrice, int32 InStock, bool bIsBuyMode, UTexture2D* InIcon = nullptr);

	/** 设置可交互状态：false=按钮禁用+背景变白 */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void SetEnabledState(bool bEnabled);

	/** 获取当前商品ID */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop|UI")
	FName GetItemID() const { return ItemID; }

	/** 点击出售按钮时广播 */
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemAction, FName, ActionItemID, bool, bBuy);

	UPROPERTY(BlueprintAssignable, Category = "Shop|UI|Events")
	FOnItemAction OnItemAction;

	/** 打开数量选择弹窗（由父级 ShopWidget 监听，在商品旁弹出小浮窗） */
	UPROPERTY(BlueprintAssignable, Category = "Shop|UI|Events")
	FOnOpenBuyCountPopup OnOpenBuyCountPopup;

	/** BuyCountWidget 类（蓝图配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|UI")
	TSubclassOf<UPP_BuyCountWidget> BuyCountWidgetClass;

	/** 设置商店和库存组件引用（由 ShopWidget 调用） */
	void SetShopAndInventory(UPP_ShopComponent* InShop, UPP_InventoryComponent* InInventory);

protected:
	virtual void NativeConstruct() override;

	// ===== 蓝图绑定控件（BindWidgetOptional） =====

	/** 物品名称文字 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ItemNameText;

	/** 价格文字 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PriceText;

	/** 库存文字 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StockText;

	/** 物品图标 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UImage> ItemIcon;

	/** 操作按钮（出售） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActionButton;

	/** 按钮文字 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ActionButtonText;

	/** 商品条目背景 Border（用于不可出售时变白） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UBorder> ItemBorder;

private:
	/** 当前商品ID */
	FName ItemID;

	/** 当前模式：true=购买 false=出售 */
	bool bBuyMode = true;

	/** 当前是否可交互 */
	bool bItemEnabled = true;

	/** 缓存的物品名称和价格 */
	int32 CachedPrice = 0;
	FText CachedName;

	UPROPERTY()
	TObjectPtr<UPP_ShopComponent> ShopComponent;

	UPROPERTY()
	TObjectPtr<UPP_InventoryComponent> InventoryComponent;

	UFUNCTION()
	void OnActionButtonClicked();

	/** 应用当前启用/禁用视觉效果 */
	void ApplyEnabledVisual();
};
