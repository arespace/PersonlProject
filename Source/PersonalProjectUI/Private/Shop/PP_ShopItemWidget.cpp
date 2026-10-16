// 商店系统 — 单个商品条目UI控件实现
// 支持启用/禁用状态，不可出售时按钮灰显、背景变白

#include "Shop/PP_ShopItemWidget.h"

#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Shop/PP_BuyCountWidget.h"
#include "Framework/PP_UIManager.h"
#include "Inventory/PP_InventoryQuery.h"

void UPP_ShopItemWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 兜底：BindWidgetOptional 要求蓝图勾选 IsVariable，若未勾选则按名字查找
	if (!ActionButton)
	{
		ActionButton = Cast<UButton>(GetWidgetFromName(TEXT("ActionButton")));
	}

	if (ActionButton)
	{
		ActionButton->OnClicked.AddDynamic(this, &UPP_ShopItemWidget::OnActionButtonClicked);
	}
}

void UPP_ShopItemWidget::SetupItem(FName InItemID, const FText& InName, int32 InPrice, int32 InStock, bool bIsBuyMode, UTexture2D* InIcon)
{
	ItemID = InItemID;
	bBuyMode = bIsBuyMode;

	// 设置图标
	if (ItemIcon && InIcon)
	{
		ItemIcon->SetBrushFromTexture(InIcon);
		ItemIcon->SetVisibility(ESlateVisibility::Visible);
	}
	else if (ItemIcon)
	{
		ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (ItemNameText)
	{
		ItemNameText->SetText(InName);
	}
	if (PriceText)
	{
		// 出售模式显示回收价，购买模式显示购买价
		if (bIsBuyMode)
		{
			PriceText->SetText(FText::Format(NSLOCTEXT("Shop", "BuyPriceFormat", "{0} 金币"), FText::AsNumber(InPrice)));
		}
		else
		{
			PriceText->SetText(FText::Format(NSLOCTEXT("Shop", "SellPriceFormat", "回收价: {0} 金币"), FText::AsNumber(InPrice)));
		}
	}
	if (StockText)
	{
		if (!bIsBuyMode)
		{
			// 出售模式：显示玩家持有数量
			StockText->SetText(FText::Format(NSLOCTEXT("Shop", "OwnedFormat", "持有: {0}"), FText::AsNumber(InStock)));
			StockText->SetVisibility(ESlateVisibility::Visible);
		}
		else if (InStock == -1)
		{
			// 购买模式 + 无限库存
			StockText->SetText(NSLOCTEXT("Shop", "StockInfinite", "库存: ∞"));
			StockText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			// 购买模式 + 有限库存
			StockText->SetText(FText::Format(NSLOCTEXT("Shop", "StockFormat", "库存: {0}"), FText::AsNumber(InStock)));
			StockText->SetVisibility(ESlateVisibility::Visible);
		}
	}
	if (ActionButtonText)
	{
		ActionButtonText->SetText(bIsBuyMode
			? NSLOCTEXT("Shop", "BuyButton", "购买")
			: NSLOCTEXT("Shop", "SellButton", "出售"));
	}

	CachedPrice = InPrice;
	CachedName = InName;
}

void UPP_ShopItemWidget::SetShopAndInventory(UObject* InShop, UObject* InInventory)
{
	ShopComponent = InShop;
	InventoryComponent = InInventory;
}

void UPP_ShopItemWidget::SetEnabledState(bool bEnabled)
{
	bItemEnabled = bEnabled;
	ApplyEnabledVisual();
}

void UPP_ShopItemWidget::ApplyEnabledVisual()
{
	static const FLinearColor EnabledColor  = FLinearColor(0.3f, 0.3f, 0.3f, 1.0f);   // 深灰（正常）
	static const FLinearColor DisabledColor = FLinearColor(0.85f, 0.85f, 0.85f, 1.0f);  // 浅灰/白（不可出售）

	// 背景颜色
	if (ItemBorder)
	{
		ItemBorder->SetBrushColor(bItemEnabled ? EnabledColor : DisabledColor);
	}

	// 按钮可交互性
	if (ActionButton)
	{
		ActionButton->SetIsEnabled(bItemEnabled);
	}
}

void UPP_ShopItemWidget::OnActionButtonClicked()
{
	if (!bItemEnabled || ItemID.IsNone())
	{
		return;
	}

	// 计算最大可选数量
	int32 MaxQty = 1;
	UObject* Inv = InventoryComponent.Get();
	if (Inv && Inv->Implements<UPP_InventoryQuery>())
	{
		if (bBuyMode)
		{
			const int32 FromGold = CachedPrice > 0
				? IPP_InventoryQuery::Execute_QueryGold(Inv) / CachedPrice
				: 0;
			MaxQty = FMath::Clamp(FromGold, 1, 999);
		}
		else
		{
			MaxQty = FMath::Max(1, IPP_InventoryQuery::Execute_QueryItemCount(Inv, ItemID));
		}
	}

	// 广播给 ShopWidget，由它在内部的 PopupContainer 中显示 BuyCount 浮窗
	OnOpenBuyCountPopup.Broadcast(ItemID, CachedName, CachedPrice, MaxQty, bBuyMode);
}
