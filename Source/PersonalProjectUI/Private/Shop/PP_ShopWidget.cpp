// 商店系统 — 商店主面板UI控件实现
// 双Tab布局：购买列表 + 出售列表

#include "Shop/PP_ShopWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBoxSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/DataTable.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Data/PP_ItemData.h"
#include "Shop/PP_ShopComponent.h"
#include "Data/PP_ShopData.h"
#include "Shop/PP_ShopItemWidget.h"
#include "Shop/PP_BuyCountWidget.h"
#include "Framework/PP_UIManager.h"
#include "Engine/GameInstance.h"
#include "GameFramework/PlayerController.h"
#include "TimerManager.h"

UPP_ShopWidget::UPP_ShopWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Overlay;
	bConsumesAllInput = true;
}

void UPP_ShopWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ResultText)
	{
		ResultText->SetVisibility(ESlateVisibility::Collapsed);
	}

	// 兜底：BindWidgetOptional 要求蓝图勾选 IsVariable，未勾选则按名字查找
	if (!CloseButton)
	{
		CloseButton = Cast<UButton>(GetWidgetFromName(TEXT("CloseButton")));
	}
	if (!BuyTabButton)
	{
		BuyTabButton = Cast<UButton>(GetWidgetFromName(TEXT("BuyTabButton")));
	}
	if (!SellTabButton)
	{
		SellTabButton = Cast<UButton>(GetWidgetFromName(TEXT("SellTabButton")));
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &UPP_ShopWidget::OnCloseButtonClicked);
		CloseButton->OnClicked.AddDynamic(this, &UPP_ShopWidget::OnCloseButtonClicked);
	}

	if (BuyTabButton)
	{
		BuyTabButton->OnClicked.RemoveDynamic(this, &UPP_ShopWidget::OnBuyTabClicked);
		BuyTabButton->OnClicked.AddDynamic(this, &UPP_ShopWidget::OnBuyTabClicked);
	}

	if (SellTabButton)
	{
		SellTabButton->OnClicked.RemoveDynamic(this, &UPP_ShopWidget::OnSellTabClicked);
		SellTabButton->OnClicked.AddDynamic(this, &UPP_ShopWidget::OnSellTabClicked);
	}
}

void UPP_ShopWidget::NativeDestruct()
{
	if (GetWorld())
	{
		GetWorld()->GetTimerManager().ClearTimer(ResultTextTimerHandle);
	}

	if (InventoryComponent)
	{
		InventoryComponent->OnGoldChanged.RemoveDynamic(this, &UPP_ShopWidget::OnGoldChanged);
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPP_ShopWidget::OnInventoryChanged);
	}
	if (ShopComponent)
	{
		ShopComponent->OnShopResult.RemoveDynamic(this, &UPP_ShopWidget::OnShopResultReceived);
		ShopComponent->OnPurchaseCompleted.RemoveDynamic(this, &UPP_ShopWidget::OnPurchaseCompleted);
	}
	Super::NativeDestruct();
}

void UPP_ShopWidget::InitializeShop(UPP_ShopComponent* InShopComponent, UPP_InventoryComponent* InInventoryComponent)
{
	ShopComponent = InShopComponent;
	InventoryComponent = InInventoryComponent; 

	if (InventoryComponent)
	{
		InventoryComponent->OnGoldChanged.RemoveDynamic(this, &UPP_ShopWidget::OnGoldChanged);
		InventoryComponent->OnGoldChanged.AddDynamic(this, &UPP_ShopWidget::OnGoldChanged);
		InventoryComponent->OnInventoryChanged.RemoveDynamic(this, &UPP_ShopWidget::OnInventoryChanged);
		InventoryComponent->OnInventoryChanged.AddDynamic(this, &UPP_ShopWidget::OnInventoryChanged);
	}

	// 绑定商店操作结果通知（购买/出售失败提示）
	if (ShopComponent)
	{
		ShopComponent->OnShopResult.RemoveDynamic(this, &UPP_ShopWidget::OnShopResultReceived);
		ShopComponent->OnShopResult.AddDynamic(this, &UPP_ShopWidget::OnShopResultReceived);
		ShopComponent->OnPurchaseCompleted.RemoveDynamic(this, &UPP_ShopWidget::OnPurchaseCompleted);
		ShopComponent->OnPurchaseCompleted.AddDynamic(this, &UPP_ShopWidget::OnPurchaseCompleted);
	}

	// 确保 Buy/Sell Tab 按钮点击事件已绑定（防止 InitializeShop 早于 NativeConstruct 被调用）
	if (BuyTabButton)
	{
		BuyTabButton->OnClicked.RemoveDynamic(this, &UPP_ShopWidget::OnBuyTabClicked);
		BuyTabButton->OnClicked.AddDynamic(this, &UPP_ShopWidget::OnBuyTabClicked);
	}
	if (SellTabButton)
	{
		SellTabButton->OnClicked.RemoveDynamic(this, &UPP_ShopWidget::OnSellTabClicked);
		SellTabButton->OnClicked.AddDynamic(this, &UPP_ShopWidget::OnSellTabClicked);
	}

	// 默认显示购买Tab
	bIsBuyTabActive = true;
	UpdateTabVisuals();
	RefreshBuyList();
	RefreshGoldText();
}

void UPP_ShopWidget::CloseShop()
{
	CloseBuyCountPopup();
	RemoveFromParent();
	// 输入模式由 UIManager 的 NativeDestruct → PopWidget → RecalculateInputMode 自动恢复
}

// ===== Tab切换 =====

void UPP_ShopWidget::OnBuyTabClicked()
{
	if (bIsBuyTabActive)
	{
		return;
	}
	CloseBuyCountPopup();
	bIsBuyTabActive = true;
	UpdateTabVisuals();
	RefreshBuyList();
}

void UPP_ShopWidget::OnSellTabClicked()
{
	if (!bIsBuyTabActive)
	{
		return;
	}
	CloseBuyCountPopup();
	bIsBuyTabActive = false;
	UpdateTabVisuals();
	RefreshSellList();
}

void UPP_ShopWidget::UpdateTabVisuals()
{
	// 蓝图侧通过按钮样式区分活跃/非活跃Tab
}

// ===== 列表刷新 =====

void UPP_ShopWidget::RefreshCurrentTab()
{
	if (bIsBuyTabActive)
	{
		RefreshBuyList();
	}
	else
	{
		RefreshSellList();
	}
}

void UPP_ShopWidget::RefreshBuyList()
{
	if (!ItemListContainer || !ShopComponent || !ShopItemWidgetClass)
	{
		return;
	}
	ItemListContainer->ClearChildren();

	const TArray<FPP_ShopItem> ShopItems = ShopComponent->GetShopItems();

	for (const FPP_ShopItem& ShopItem : ShopItems)
	{
		if (ShopItem.ItemID.IsNone())
		{
			continue;
		}

		int32 BuyPrice = ShopComponent->GetItemBuyPrice(ShopItem.ItemID);
		FText DisplayName = FText::FromName(ShopItem.ItemID);
		UTexture2D* Icon = nullptr;

		if (InventoryComponent)
		{
			if (const FPP_ItemInfo* Info = InventoryComponent->FindItemInfo(ShopItem.ItemID))
			{
				DisplayName = Info->DisplayName;
				Icon = Info->Icon.LoadSynchronous();
			}
		}

		UPP_ShopItemWidget* ItemWidget = CreateWidget<UPP_ShopItemWidget>(this, ShopItemWidgetClass);
		if (!ItemWidget)
		{
			continue;
		}

		ItemWidget->SetupItem(ShopItem.ItemID, DisplayName, BuyPrice, ShopItem.Stock, /*bIsBuyMode=*/true, Icon);
		ItemWidget->SetShopAndInventory(ShopComponent, InventoryComponent);

		// Stock == 0 时不可购买（Stock == -1 表示无限供应，可交互）
		const bool bCanBuy = (ShopItem.Stock != 0);
		ItemWidget->SetEnabledState(bCanBuy);

		// 绑定委托：点击购买/出售时，在 PopupContainer 中弹出数量选择浮窗
		ItemWidget->OnOpenBuyCountPopup.AddDynamic(this, &UPP_ShopWidget::HandleOpenBuyCountPopup);

		ItemListContainer->AddChild(ItemWidget);

		// 确保 WrapBox 模式下不填充剩余空间
		if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(ItemWidget->Slot))
		{
			WrapSlot->SetFillEmptySpace(false);
		}
	}

	InvalidateLayoutChain();
}

void UPP_ShopWidget::RefreshSellList()
{
	if (!ItemListContainer || !InventoryComponent || !ShopItemWidgetClass)
	{
		return;
	}
	ItemListContainer->ClearChildren();

	const float SellMul = ShopComponent ? ShopComponent->GetSellPriceMultiplier() : 0.5f;
	const TArray<FPP_ItemInstance> PlayerItems = InventoryComponent->GetItems();

	for (const FPP_ItemInstance& Inst : PlayerItems)
	{
		if (Inst.ItemID.IsNone() || Inst.Count <= 0)
		{
			continue;
		}

		const FPP_ItemInfo* Info = InventoryComponent->FindItemInfo(Inst.ItemID);
		if (!Info)
		{
			continue;
		}

		// 过滤不可出售物品（SellPrice <= 0）
		if (Info->SellPrice <= 0)
		{
			continue;
		}

		const int32 ActualSellPrice = FMath::FloorToInt(static_cast<float>(Info->SellPrice) * SellMul);

		UPP_ShopItemWidget* ItemWidget = CreateWidget<UPP_ShopItemWidget>(this, ShopItemWidgetClass);
		if (!ItemWidget)
		{
			continue;
		}

		ItemWidget->SetupItem(Inst.ItemID, Info->DisplayName, ActualSellPrice, Inst.Count, /*bIsBuyMode=*/false, Info->Icon.LoadSynchronous());
		ItemWidget->SetShopAndInventory(ShopComponent, InventoryComponent);
		ItemWidget->SetEnabledState(true);

		// 绑定委托：点击出售时，在 PopupContainer 中弹出数量选择浮窗
		ItemWidget->OnOpenBuyCountPopup.AddDynamic(this, &UPP_ShopWidget::HandleOpenBuyCountPopup);
		ItemListContainer->AddChild(ItemWidget);

		// 确保 WrapBox 模式下不填充剩余空间
		if (UWrapBoxSlot* WrapSlot = Cast<UWrapBoxSlot>(ItemWidget->Slot))
		{
			WrapSlot->SetFillEmptySpace(false);
		}
	}

	InvalidateLayoutChain();
}

// ===== 金币 =====

void UPP_ShopWidget::RefreshGoldText()
{
	if (GoldText && InventoryComponent)
	{
		GoldText->SetText(FText::Format(NSLOCTEXT("Shop", "GoldFormat", "金币: {0}"),
			FText::AsNumber(InventoryComponent->GetGold())));
	}
}

// ===== 委托回调 =====

void UPP_ShopWidget::OnGoldChanged(int32 NewGold)
{
	RefreshGoldText();
}

void UPP_ShopWidget::OnInventoryChanged(FName ItemID, int32 NewCount)
{
	CloseBuyCountPopup();
	RefreshCurrentTab();
}

void UPP_ShopWidget::OnCloseButtonClicked()
{
	CloseShop();
}

void UPP_ShopWidget::OnShopResultReceived(const FText& Message)
{
	if (ResultText)
	{
		ResultText->SetText(Message);
		ResultText->SetVisibility(ESlateVisibility::Visible);

		// 3 秒后自动隐藏
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(ResultTextTimerHandle);
			World->GetTimerManager().SetTimer(ResultTextTimerHandle, [this]()
			{
				if (ResultText)
				{
					ResultText->SetVisibility(ESlateVisibility::Collapsed);
				}
			}, 3.0f, false);
		}
	}
}

// ===== BuyCount 浮窗 =====

void UPP_ShopWidget::HandleOpenBuyCountPopup(FName ItemID, const FText& ItemName, int32 UnitPrice, int32 MaxCount, bool bBuyMode)
{
	// 如果已有弹窗打开，先关闭
	CloseBuyCountPopup();

	if (!BuyCountWidgetClass)
	{
		return;
	}

	// 创建 BuyCountWidget
	ActiveBuyCountWidget = CreateWidget<UPP_BuyCountWidget>(this, BuyCountWidgetClass);
	if (!ActiveBuyCountWidget)
	{
		return;
	}

	ActiveBuyCountWidget->SetupCountPicker(ItemID, ItemName, UnitPrice, MaxCount,
		bBuyMode, ShopComponent, InventoryComponent);

	// 确认或取消后自动关闭弹窗
	ActiveBuyCountWidget->OnCountConfirmed.AddDynamic(this, &UPP_ShopWidget::OnBuyCountConfirmed);

	// 加入到 PopupContainer，定位在点击的商品条目旁边
	if (PopupContainer)
	{
		PopupContainer->ClearChildren();
		PopupContainer->AddChild(ActiveBuyCountWidget);

		// 查找被点击的商品条目，获取其在屏幕上的位置
		FVector2D ItemPos = FVector2D::ZeroVector;
		for (int32 i = 0; i < ItemListContainer->GetChildrenCount(); ++i)
		{
			if (UPP_ShopItemWidget* Child = Cast<UPP_ShopItemWidget>(ItemListContainer->GetChildAt(i)))
			{
				if (Child->GetItemID() == ItemID)
				{
					const FGeometry& Geo = PopupContainer->GetCachedGeometry();
					const FVector2D ContainerOrigin = Geo.GetAbsolutePosition();
					const FVector2D ItemCenter = Child->GetCachedGeometry().GetAbsolutePosition() +
						Child->GetCachedGeometry().GetLocalSize() * 0.5f;
					ItemPos = ItemCenter - ContainerOrigin;
					break;
				}
			}
		}

		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ActiveBuyCountWidget->Slot))
		{
			CanvasSlot->SetAutoSize(true);
			CanvasSlot->SetPosition(ItemPos + FVector2D(50.0f, -50.0f));
		}
	}
}

void UPP_ShopWidget::CloseBuyCountPopup()
{
	if (PopupContainer)
	{
		PopupContainer->ClearChildren();
	}
	ActiveBuyCountWidget = nullptr;
}

void UPP_ShopWidget::OnBuyCountConfirmed(FName ItemID, int32 Count, bool bBuy)
{
	CloseBuyCountPopup();
	// 立即刷新列表以反映库存变化
	RefreshCurrentTab();
}

void UPP_ShopWidget::OnPurchaseCompleted(AActor* Buyer, FName ItemID, int32 Count)
{
	// 服务端购买完成后立即刷新列表（确保库存扣减已反映在 DataTable 中）
	RefreshCurrentTab();
}

void UPP_ShopWidget::InvalidateLayoutChain()
{
	UWidget* WidgetToInvalidate = ItemListContainer;
	while (WidgetToInvalidate)
	{
		WidgetToInvalidate->InvalidateLayoutAndVolatility();
		WidgetToInvalidate = WidgetToInvalidate->GetParent();
	}
}