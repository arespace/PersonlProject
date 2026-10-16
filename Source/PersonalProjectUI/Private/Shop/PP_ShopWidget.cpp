// 商店系统 — 商店主面板UI控件实现
// 双Tab布局：购买列表 + 出售列表

#include "Shop/PP_ShopWidget.h"

#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/WrapBoxSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Engine/DataTable.h"
#include "Inventory/PP_InventoryQuery.h"
#include "Data/PP_ItemData.h"
#include "Shop/PP_ShopQuery.h"
#include "Data/PP_ShopData.h"
#include "Shop/PP_ShopItemWidget.h"
#include "Shop/PP_BuyCountWidget.h"
#include "Framework/PP_UIManager.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_DataManager.h"
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

	Super::NativeDestruct();
}

void UPP_ShopWidget::InitializeShop(UObject* InShopComponent, UObject* InInventoryComponent)
{
	ShopComponent = InShopComponent;
	InventoryComponent = InInventoryComponent;

	// 退订旧订阅，避免重复
	UnsubscribeEventBus();

	// 句柄由基类托管，NativeDestruct 自动退订
	SubscribeEventBus<FPP_InventoryUpdatedEvent>(
		[this](const FPP_InventoryUpdatedEvent& Event) { HandleInventoryUpdated(Event); });
	SubscribeEventBus<FPP_GoldUpdatedEvent>(
		[this](const FPP_GoldUpdatedEvent& Event) { HandleGoldUpdated(Event); });
	SubscribeEventBus<FPP_ShopResultEvent>(
		[this](const FPP_ShopResultEvent& Event) { HandleShopResult(Event); });
	SubscribeEventBus<FPP_PurchaseCompletedEvent>(
		[this](const FPP_PurchaseCompletedEvent& Event) { HandlePurchaseCompleted(Event); });

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
	UObject* Shop = ShopComponent.Get();
	if (!Shop || !Shop->Implements<UPP_ShopQuery>())
	{
		return;
	}

	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	const TArray<FPP_ShopItem> ShopItems = IPP_ShopQuery::Execute_QueryShopItems(Shop);

	TArray<FShopListEntry> Entries;
	for (const FPP_ShopItem& ShopItem : ShopItems)
	{
		if (ShopItem.ItemID.IsNone())
		{
			continue;
		}

		FShopListEntry Entry;
		Entry.ItemID = ShopItem.ItemID;
		Entry.DisplayName = FText::FromName(ShopItem.ItemID);
		Entry.Price = IPP_ShopQuery::Execute_QueryItemBuyPrice(Shop, ShopItem.ItemID);
		Entry.Count = ShopItem.Stock;
		Entry.bIsBuyMode = true;
		// Stock == 0 时不可购买（Stock == -1 表示无限供应，可交互）
		Entry.bEnabled = (ShopItem.Stock != 0);

		if (DataManager)
		{
			if (const FPP_ItemInfo* Info = DataManager->GetItemInfo(ShopItem.ItemID))
			{
				Entry.DisplayName = Info->DisplayName;
				Entry.Icon = Info->Icon.LoadSynchronous();
			}
		}
		Entries.Add(Entry);
	}

	RebuildItemList(Entries);
}

void UPP_ShopWidget::RefreshSellList()
{
	UObject* Inv = InventoryComponent.Get();
	if (!Inv || !Inv->Implements<UPP_InventoryQuery>())
	{
		return;
	}

	UObject* Shop = ShopComponent.Get();
	const float SellMul = (Shop && Shop->Implements<UPP_ShopQuery>())
		? IPP_ShopQuery::Execute_QuerySellPriceMultiplier(Shop)
		: 0.5f;

	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	const TArray<FPP_ItemInstance> PlayerItems = IPP_InventoryQuery::Execute_QueryItems(Inv);

	TArray<FShopListEntry> Entries;
	for (const FPP_ItemInstance& Inst : PlayerItems)
	{
		if (Inst.ItemID.IsNone() || Inst.Count <= 0)
		{
			continue;
		}

		const FPP_ItemInfo* Info = DataManager ? DataManager->GetItemInfo(Inst.ItemID) : nullptr;
		// 无配置或不可出售（SellPrice <= 0）的物品跳过
		if (!Info || Info->SellPrice <= 0)
		{
			continue;
		}

		FShopListEntry Entry;
		Entry.ItemID = Inst.ItemID;
		Entry.DisplayName = Info->DisplayName;
		Entry.Price = FMath::FloorToInt(static_cast<float>(Info->SellPrice) * SellMul);
		Entry.Count = Inst.Count;
		Entry.bIsBuyMode = false;
		Entry.bEnabled = true;
		Entry.Icon = Info->Icon.LoadSynchronous();
		Entries.Add(Entry);
	}

	RebuildItemList(Entries);
}

void UPP_ShopWidget::RebuildItemList(const TArray<FShopListEntry>& Entries)
{
	if (!ItemListContainer || !ShopItemWidgetClass)
	{
		return;
	}
	ItemListContainer->ClearChildren();

	UObject* Shop = ShopComponent.Get();
	UObject* Inv = InventoryComponent.Get();

	for (const FShopListEntry& Entry : Entries)
	{
		UPP_ShopItemWidget* ItemWidget = CreateWidget<UPP_ShopItemWidget>(this, ShopItemWidgetClass);
		if (!ItemWidget)
		{
			continue;
		}

		ItemWidget->SetupItem(Entry.ItemID, Entry.DisplayName, Entry.Price, Entry.Count, Entry.bIsBuyMode, Entry.Icon);
		ItemWidget->SetShopAndInventory(Shop, Inv);
		ItemWidget->SetEnabledState(Entry.bEnabled);

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

// ===== 金币 =====

void UPP_ShopWidget::RefreshGoldText()
{
	UObject* Inv = InventoryComponent.Get();
	if (GoldText && Inv && Inv->Implements<UPP_InventoryQuery>())
	{
		GoldText->SetText(FText::Format(NSLOCTEXT("Shop", "GoldFormat", "金币: {0}"),
			FText::AsNumber(IPP_InventoryQuery::Execute_QueryGold(Inv))));
	}
}

// ===== 事件回调 =====

void UPP_ShopWidget::HandleInventoryUpdated(const FPP_InventoryUpdatedEvent& Event)
{
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}
	CloseBuyCountPopup();
	RefreshCurrentTab();
}

void UPP_ShopWidget::HandleGoldUpdated(const FPP_GoldUpdatedEvent& Event)
{
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}
	RefreshGoldText();
}

void UPP_ShopWidget::HandleShopResult(const FPP_ShopResultEvent& Event)
{
	if (ResultText)
	{
		ResultText->SetText(Event.Message);
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

void UPP_ShopWidget::HandlePurchaseCompleted(const FPP_PurchaseCompletedEvent& Event)
{
	// 服务端购买完成后刷新列表（确保库存扣减已反映在 DataTable 中）
	RefreshCurrentTab();
}

void UPP_ShopWidget::OnCloseButtonClicked()
{
	CloseShop();
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
		bBuyMode, ShopComponent.Get(), InventoryComponent.Get());

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

void UPP_ShopWidget::InvalidateLayoutChain()
{
	UWidget* WidgetToInvalidate = ItemListContainer;
	while (WidgetToInvalidate)
	{
		WidgetToInvalidate->InvalidateLayoutAndVolatility();
		WidgetToInvalidate = WidgetToInvalidate->GetParent();
	}
}