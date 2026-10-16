// 商店系统 — HUD金币常驻显示控件实现

#include "Shop/PP_GoldDisplayWidget.h"

#include "Components/TextBlock.h"
#include "Inventory/PP_InventoryComponent.h"

void UPP_GoldDisplayWidget::NativeDestruct()
{
	if (Inventory)
	{
		Inventory->OnGoldChanged.RemoveDynamic(this, &UPP_GoldDisplayWidget::HandleGoldChanged);
	}
	Super::NativeDestruct();
}

void UPP_GoldDisplayWidget::BindInventoryComponent(UPP_InventoryComponent* InInventory)
{
	// 解绑旧的避免重复
	if (Inventory)
	{
		Inventory->OnGoldChanged.RemoveDynamic(this, &UPP_GoldDisplayWidget::HandleGoldChanged);
	}

	Inventory = InInventory;
	if (Inventory)
	{
		Inventory->OnGoldChanged.RemoveDynamic(this, &UPP_GoldDisplayWidget::HandleGoldChanged);
		Inventory->OnGoldChanged.AddDynamic(this, &UPP_GoldDisplayWidget::HandleGoldChanged);
		// 立即刷新初值
		UpdateGoldDisplay(Inventory->GetGold());
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

void UPP_GoldDisplayWidget::HandleGoldChanged(int32 NewGold)
{
	UpdateGoldDisplay(NewGold);
}
