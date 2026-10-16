// 商店系统 — 数量选择弹窗实现

#include "Shop/PP_BuyCountWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerController.h"
#include "Shop/PP_ShopActions.h"

UPP_BuyCountWidget::UPP_BuyCountWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UPP_BuyCountWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 兜底：BindWidgetOptional 要求蓝图勾选 IsVariable，未勾选则按名字查找
    if (!MinusButton)
    {
        MinusButton = Cast<UButton>(GetWidgetFromName(TEXT("MinusButton")));
    }
    if (!PlusButton)
    {
        PlusButton = Cast<UButton>(GetWidgetFromName(TEXT("PlusButton")));
    }
    if (!ConfirmButton)
    {
        ConfirmButton = Cast<UButton>(GetWidgetFromName(TEXT("ConfirmButton")));
    }
    if (!CancelButton)
    {
        CancelButton = Cast<UButton>(GetWidgetFromName(TEXT("CancelButton")));
    }

    if (MinusButton)
    {
        MinusButton->OnClicked.AddDynamic(this, &UPP_BuyCountWidget::OnMinusClicked);
    }
    if (PlusButton)
    {
        PlusButton->OnClicked.AddDynamic(this, &UPP_BuyCountWidget::OnPlusClicked);
    }
    if (ConfirmButton)
    {
        ConfirmButton->OnClicked.AddDynamic(this, &UPP_BuyCountWidget::OnConfirmClicked);
    }
    if (CancelButton)
    {
        CancelButton->OnClicked.AddDynamic(this, &UPP_BuyCountWidget::OnCancelClicked);
    }
}

void UPP_BuyCountWidget::SetupCountPicker(FName InItemID,
                                           const FText& InItemName,
                                           int32 InUnitPrice,
                                           int32 InMaxCount,
                                           bool bInBuyMode,
                                           UObject* InShopComponent,
                                           UObject* InInventory)
{
    ItemID = InItemID;
    UnitPrice = InUnitPrice;
    MaxCount = FMath::Max(1, InMaxCount);
    CurrentCount = FMath::Clamp(1, 1, MaxCount);
    bBuyMode = bInBuyMode;
    ShopComponent = InShopComponent;
    Inventory = InInventory;

    if (ItemNameText)
    {
        ItemNameText->SetText(InItemName);
    }

    RefreshDisplay();
}

void UPP_BuyCountWidget::RefreshDisplay()
{
    if (UnitPriceText)
    {
        UnitPriceText->SetText(FText::Format(
            NSLOCTEXT("Shop", "UnitPriceFmt", "单价: {0}G"),
            FText::AsNumber(UnitPrice)));
    }

    if (CountText)
    {
        CountText->SetText(FText::AsNumber(CurrentCount));
    }

    if (TotalPriceText)
    {
        TotalPriceText->SetText(FText::Format(
            NSLOCTEXT("Shop", "TotalPriceFmt", "总价: {0}G"),
            FText::AsNumber(UnitPrice * CurrentCount)));
    }
}

void UPP_BuyCountWidget::OnMinusClicked()
{
    if (CurrentCount > 1)
    {
        --CurrentCount;
        RefreshDisplay();
    }
}

void UPP_BuyCountWidget::OnPlusClicked()
{
    if (CurrentCount < MaxCount)
    {
        ++CurrentCount;
        RefreshDisplay();
    }
}

void UPP_BuyCountWidget::OnConfirmClicked()
{
    UObject* Shop = ShopComponent.Get();
    if (!Shop || !Shop->Implements<UPP_ShopActions>() || ItemID.IsNone() || CurrentCount <= 0)
    {
        return;
    }

    APlayerController* PC = GetOwningPlayer();
    if (!PC)
    {
        return;
    }

    if (bBuyMode)
    {
        IPP_ShopActions::Execute_RequestBuyItem(Shop, PC, ItemID, CurrentCount);
    }
    else
    {
        IPP_ShopActions::Execute_RequestSellItem(Shop, PC, ItemID, CurrentCount);
    }

    OnCountConfirmed.Broadcast(ItemID, CurrentCount, bBuyMode);

    // 关闭弹窗（从父控件移除，由 ShopWidget 管理）
    RemoveFromParent();
}

void UPP_BuyCountWidget::OnCancelClicked()
{
    RemoveFromParent();
}

