// 商店系统 — 购买/出售数量选择弹窗

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_BuyCountWidget.generated.h"

class UTextBlock;
class UButton;

/** 数量确认回调：ItemID, 数量, 是否为购买模式 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnCountConfirmed, FName, ItemID, int32, Count, bool, bBuy);

/**
 * 数量选择弹窗
 *
 * OwningLayer = Modal, bConsumesAllInput = true
 *
 * 由 ShopItemWidget 点击触发 → 弹出此窗口
 * 确认后通过 IPP_ShopActions 发起购买/出售请求
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECTUI_API UPP_BuyCountWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UPP_BuyCountWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

    /**
     * 初始化数量选择器
     * @param InItemID        物品ID
     * @param InItemName      物品显示名称
     * @param InUnitPrice     单价
     * @param InMaxCount      最大可选数量
     * @param bInBuyMode      true=购买模式, false=出售模式
     * @param InShopComponent 商店数据源（实现 IPP_ShopActions，确认时发起请求）
     * @param InInventory     玩家库存数据源（实现 IPP_InventoryQuery）
     */
    UFUNCTION(BlueprintCallable, Category = "Shop|UI")
    void SetupCountPicker(FName InItemID,
                          const FText& InItemName,
                          int32 InUnitPrice,
                          int32 InMaxCount,
                          bool bInBuyMode,
                          UObject* InShopComponent,
                          UObject* InInventory);

    /** 数量确认事件（供 ShopWidget 监听刷新） */
    UPROPERTY(BlueprintAssignable, Category = "Shop|UI|Events")
    FOnCountConfirmed OnCountConfirmed;

protected:
    virtual void NativeConstruct() override;

    // ===== 蓝图绑定控件 =====

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> ItemNameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> UnitPriceText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> CountText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TotalPriceText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> MinusButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> PlusButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> ConfirmButton;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UButton> CancelButton;

private:
    // ===== 按钮回调 =====

    UFUNCTION()
    void OnMinusClicked();

    UFUNCTION()
    void OnPlusClicked();

    UFUNCTION()
    void OnConfirmClicked();

    UFUNCTION()
    void OnCancelClicked();

    /** 刷新显示（数量变化时） */
    void RefreshDisplay();

    // ===== 状态 =====

    FName ItemID;
    int32 UnitPrice = 0;
    int32 CurrentCount = 1;
    int32 MaxCount = 1;
    bool bBuyMode = true;

    UPROPERTY()
    TWeakObjectPtr<UObject> ShopComponent;

    UPROPERTY()
    TWeakObjectPtr<UObject> Inventory;
};
