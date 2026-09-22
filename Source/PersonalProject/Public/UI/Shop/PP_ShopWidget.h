// 商店系统 — 商店主面板UI控件
// 双Tab布局：可购买列表 / 可出售列表

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "PP_ShopWidget.generated.h"

class UPP_ShopComponent;
class UPP_InventoryComponent;
class UPP_ShopItemWidget;
class UPP_BuyCountWidget;
class UPanelWidget;
class UTextBlock;
class UButton;

/**
 * 商店主面板 Widget
 *
 * 职责：
 *   1. Buy/Sell Tab切换，默认显示购买列表
 *   2. 购买列表：显示商店商品，库存>=1可交互，库存=0灰白不可交互
 *   3. 出售列表：显示玩家背包中可出售物品，点击出售移除物品+加金币
 *   4. 底部金币显示，监听库存变化实时刷新
 */
UCLASS()
class PERSONALPROJECT_API UPP_ShopWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_ShopWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void InitializeShop(UPP_ShopComponent* InShopComponent, UPP_InventoryComponent* InInventoryComponent);

	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void CloseShop();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ===== 蓝图绑定控件 =====

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ShopNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ItemListContainer;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** 操作结果提示文字（购买/出售失败时显示，几秒后自动隐藏） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> ResultText;

	/** 购买Tab按钮 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> BuyTabButton;

	/** 出售Tab按钮 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SellTabButton;

	/** 弹窗浮层容器（用于在商品旁弹出 BuyCount 小浮窗，蓝图需绑定为 CanvasPanel） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> PopupContainer;

	/** 商品条目 Widget 类（蓝图配置） */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|UI")
	TSubclassOf<UPP_ShopItemWidget> ShopItemWidgetClass;

	/** 数量选择弹窗 Widget 类（蓝图配置） */
	UPROPERTY(EditDefaultsOnly, Category = "Shop|UI")
	TSubclassOf<UPP_BuyCountWidget> BuyCountWidgetClass;

private:
	UPROPERTY()
	TObjectPtr<UPP_ShopComponent> ShopComponent;

	UPROPERTY()
	TObjectPtr<UPP_InventoryComponent> InventoryComponent;

	/** 当前活跃Tab：true=购买, false=出售 */
	bool bIsBuyTabActive = true;

	/** 刷新当前活跃Tab的列表 */
	void RefreshCurrentTab();

	/** 刷新购买列表（数据源：ShopComponent） */
	void RefreshBuyList();

	/** 刷新出售列表（数据源：InventoryComponent） */
	void RefreshSellList();

	/** 刷新金币显示 */
	void RefreshGoldText();

	/** 更新Tab按钮的高亮/默认视觉 */
	void UpdateTabVisuals();

	// ===== 委托回调 =====

	UFUNCTION()
	void OnGoldChanged(int32 NewGold);

	UFUNCTION()
	void OnInventoryChanged(FName ItemID, int32 NewCount);

	UFUNCTION()
	void OnBuyTabClicked();

	UFUNCTION()
	void OnSellTabClicked();

	UFUNCTION()
	void OnCloseButtonClicked();

	/** 商店操作结果回调（金币不足、库存不足等） */
	UFUNCTION()
	void OnShopResultReceived(const FText& Message);

	// ===== BuyCount 浮窗 =====

	/** 处理商品条目发起的打开数量选择器请求（在商品旁弹出小浮窗） */
	UFUNCTION()
	void HandleOpenBuyCountPopup(FName ItemID, const FText& ItemName, int32 UnitPrice, int32 MaxCount, bool bBuyMode);

	/** 关闭当前打开的 BuyCount 浮窗 */
	void CloseBuyCountPopup();

	/** BuyCount 确认/取消回调桥接（签名匹配 FOnCountConfirmed） */
	UFUNCTION()
	void OnBuyCountConfirmed(FName ItemID, int32 Count, bool bBuy);

	/** 购买完成后刷新列表（绑定到 ShopComponent->OnPurchaseCompleted） */
	UFUNCTION()
	void OnPurchaseCompleted(AActor* Buyer, FName ItemID, int32 Count);

	/** 当前打开的 BuyCount 浮窗引用 */
	UPROPERTY()
	TObjectPtr<UPP_BuyCountWidget> ActiveBuyCountWidget;

	/** 自动隐藏 ResultText 的定时器句柄 */
	FTimerHandle ResultTextTimerHandle;

	/** 沿父链向上失效布局缓存（修复 WrapBox 切换 Tab 后的布局异常） */
	void InvalidateLayoutChain();
};
