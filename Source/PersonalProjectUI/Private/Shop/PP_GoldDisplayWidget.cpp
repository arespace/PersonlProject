// 商店系统 — HUD金币常驻显示控件实现

#include "Shop/PP_GoldDisplayWidget.h"

#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryQuery.h"
#include "Framework/PP_EventBus.h"
#include "Engine/GameInstance.h"

void UPP_GoldDisplayWidget::NativeDestruct()
{
	if (UPP_EventBus* EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPP_EventBus>() : nullptr)
	{
		EventBus->Unsubscribe(GoldUpdatedHandle);
	}
	GoldUpdatedHandle.Reset();

	Super::NativeDestruct();
}

void UPP_GoldDisplayWidget::BindInventoryComponent(UObject* InInventory)
{
	UPP_EventBus* EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPP_EventBus>() : nullptr;

	// 解绑旧的订阅，避免重复
	if (EventBus)
	{
		EventBus->Unsubscribe(GoldUpdatedHandle);
	}
	GoldUpdatedHandle.Reset();

	Inventory = InInventory;

	if (InInventory && InInventory->Implements<UPP_InventoryQuery>() && EventBus)
	{
		GoldUpdatedHandle = EventBus->Subscribe<FPP_GoldUpdatedEvent>(
			[this](const FPP_GoldUpdatedEvent& Event) { HandleGoldUpdated(Event); });

		// 立即刷新初值
		UpdateGoldDisplay(IPP_InventoryQuery::Execute_QueryGold(InInventory));
	}
	else
	{
		UpdateGoldDisplay(0);
	}
}

void UPP_GoldDisplayWidget::UpdateGoldDisplay(int32 NewGold)
{
	if (GoldText)
	{
		GoldText->SetText(FText::Format(NSLOCTEXT("Shop", "GoldHUDFormat", "金币: {0}"),
			FText::AsNumber(NewGold)));
	}
}

void UPP_GoldDisplayWidget::HandleGoldUpdated(const FPP_GoldUpdatedEvent& Event)
{
	// 只处理本地玩家的金币变更（EventBus 是 GameInstance 级，可能收到其他玩家的广播）
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}
	UpdateGoldDisplay(Event.NewGold);
}
