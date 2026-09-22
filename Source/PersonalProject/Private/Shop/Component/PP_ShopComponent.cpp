// 商店系统 — 商店管理组件实现
// 关键逻辑：购买/出售验证、库存联动、价格计算、事件广播

#include "Shop/Component/PP_ShopComponent.h"

#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/Component/PP_InventoryComponent.h"
#include "Inventory/Data/PP_ItemData.h"
#include "Player/PP_PlayerState.h"

UPP_ShopComponent::UPP_ShopComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(false);
	SellPriceMultiplier = 0.5f;
}

// 静态委托定义
FOnPurchaseCompleted UPP_ShopComponent::OnPurchaseCompletedDelegate;

// ===== 内部辅助 =====

UPP_InventoryComponent* UPP_ShopComponent::GetPlayerInventory(APlayerController* PC) const
{
	if (!IsValid(PC))
	{
		return nullptr;
	}
	APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>();
	if (!IsValid(PS))
	{
		return nullptr;
	}
	return PS->GetInventoryComponent();
}

FPP_ShopItem* UPP_ShopComponent::FindShopItem(FName ItemID) const
{
	if (!ShopDataTable || ItemID.IsNone())
	{
		return nullptr;
	}
	static const FString ContextString(TEXT("UPP_ShopComponent::FindShopItem"));
	// 遍历所有行，按 ItemID 字段匹配（因为数据表行名如 Shop_Armor_Leath 与实际 ItemID 不一致）
	TArray<FName> RowNames = ShopDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		FPP_ShopItem* Row = ShopDataTable->FindRow<FPP_ShopItem>(RowName, ContextString, false);
		if (Row && Row->ItemID == ItemID)
		{
			return Row;
		}
	}
	return nullptr;
}

TArray<FPP_ShopItem> UPP_ShopComponent::GetShopItems() const
{
	TArray<FPP_ShopItem> Result;
	if (!ShopDataTable)
	{
		return Result;
	}
	static const FString ContextString(TEXT("UPP_ShopComponent::GetShopItems"));
	TArray<FName> RowNames = ShopDataTable->GetRowNames();
	for (const FName& RowName : RowNames)
	{
		if (FPP_ShopItem* Row = ShopDataTable->FindRow<FPP_ShopItem>(RowName, ContextString, false))
		{
			Result.Add(*Row);
		}
	}
	return Result;
}

int32 UPP_ShopComponent::GetItemBuyPrice(FName ItemID) const
{
	const FPP_ShopItem* ShopItem = FindShopItem(ItemID);
	if (ShopItem && ShopItem->PriceOverride > 0)
	{
		return ShopItem->PriceOverride;
	}
	if (ItemDataTable)
	{
		static const FString ContextString(TEXT("UPP_ShopComponent::GetItemBuyPrice"));
		if (const FPP_ItemInfo* Info = ItemDataTable->FindRow<FPP_ItemInfo>(ItemID, ContextString, false))
		{
			return Info->BuyPrice;
		}
	}
	return 0;
}

// ===== 购买 =====

void UPP_ShopComponent::Server_BuyItem_Implementation(APlayerController* Buyer, FName ItemID, int32 Count)
{
	// 服务器权威检查：仅在服务端执行实际逻辑
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!IsValid(Buyer) || ItemID.IsNone() || Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem 参数无效"));
		return;
	}

	// 1) 验证商品存在性
	FPP_ShopItem* ShopItem = FindShopItem(ItemID);
	if (!ShopItem)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem: 商店未售卖商品 %s"), *ItemID.ToString());
		return;
	}

	// 2) 验证库存充足（-1表示无限）
	if (ShopItem->Stock != -1 && ShopItem->Stock < Count)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem: 商品 %s 库存不足，剩余 %d 需要 %d"),
			*ItemID.ToString(), ShopItem->Stock, Count);
		Client_OnShopResult(NSLOCTEXT("Shop", "StockInsufficient", "库存不足"));
		return;
	}

	// 3) 计算总价并验证金币
	const int32 UnitPrice = GetItemBuyPrice(ItemID);
	if (UnitPrice <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem: 商品 %s 价格未配置"), *ItemID.ToString());
		return;
	}
	const int32 TotalPrice = UnitPrice * Count;

	UPP_InventoryComponent* Inventory = GetPlayerInventory(Buyer);
	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem: 找不到购买者的库存组件"));
		return;
	}
	if (Inventory->GetGold() < TotalPrice)
	{
		UE_LOG(LogTemp, Log, TEXT("[Shop] BuyItem: 金币不足，需要 %d 当前 %d"),
			TotalPrice, Inventory->GetGold());
		Client_OnShopResult(FText::Format(
			NSLOCTEXT("Shop", "GoldInsufficient", "金币不足，需要 {0}，当前 {1}"),
			FText::AsNumber(TotalPrice), FText::AsNumber(Inventory->GetGold())));
		return;
	}

	// 4) 扣金币 → 加物品
	if (!Inventory->SpendGold(TotalPrice))
	{
		return;
	}

	// 预检：确认 Inventory 的 ItemDataTable 能找到该物品（避免 AddItem 失败后猜测原因）
	const FPP_ItemInfo* ItemInfo = nullptr;
	if (ItemDataTable)
	{
		static const FString BuyCtx(TEXT("UPP_ShopComponent::Server_BuyItem"));
		ItemInfo = ItemDataTable->FindRow<FPP_ItemInfo>(ItemID, BuyCtx, false);
	}
	if (!ItemInfo)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem: ItemDataTable 中找不到物品 %s，请检查 PP_InventoryComponent 的 ItemDataTable 配置"), *ItemID.ToString());
		Inventory->AddGold(TotalPrice);
		Client_OnShopResult(FText::Format(
			NSLOCTEXT("Shop", "ItemNotFound", "物品 {0} 配置缺失"),
			FText::FromName(ItemID)));
		return;
	}

	if (!Inventory->AddItem(ItemID, Count))
	{
		// 物品添加失败，回滚金币
		Inventory->AddGold(TotalPrice);
		UE_LOG(LogTemp, Warning, TEXT("[Shop] BuyItem: 物品 %s 添加失败（可能背包已满或无服务端权限），已回滚金币"), *ItemID.ToString());
		Client_OnShopResult(NSLOCTEXT("Shop", "BackpackFull", "背包空间不足"));
		return;
	}

	// 5) 扣减商店库存（仅有限库存时）
	if (ShopItem->Stock != -1)
	{
		ShopItem->Stock -= Count;
	}

	// 6) 广播购买完成事件（实例委托：UI 刷新；静态委托：任务系统监听）
	OnPurchaseCompleted.Broadcast(Buyer, ItemID, Count);
	OnPurchaseCompletedDelegate.Broadcast(Buyer, ItemID, Count);

	UE_LOG(LogTemp, Log, TEXT("[Shop] BuyItem: %s 购买 %s x%d 花费 %d"),
		*Buyer->GetName(), *ItemID.ToString(), Count, TotalPrice);
}

// ===== 出售 =====

void UPP_ShopComponent::Server_SellItem_Implementation(APlayerController* Seller, FName ItemID, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}
	if (!IsValid(Seller) || ItemID.IsNone() || Count <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] SellItem 参数无效"));
		return;
	}

	UPP_InventoryComponent* Inventory = GetPlayerInventory(Seller);
	if (!Inventory)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] SellItem: 找不到出售者的库存组件"));
		return;
	}

	// 1) 验证背包中有足够物品
	if (!Inventory->HasItem(ItemID, Count))
	{
		UE_LOG(LogTemp, Log, TEXT("[Shop] SellItem: 背包物品不足"));
		return;
	}

	// 2) 查询物品 SellPrice
	int32 SellPriceUnit = 0;
	if (ItemDataTable)
	{
		static const FString ContextString(TEXT("UPP_ShopComponent::Server_SellItem"));
		if (const FPP_ItemInfo* Info = ItemDataTable->FindRow<FPP_ItemInfo>(ItemID, ContextString, false))
		{
			SellPriceUnit = Info->SellPrice;
		}
	}
	if (SellPriceUnit <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] SellItem: 物品 %s 出售价格未配置"), *ItemID.ToString());
		return;
	}

	// 3) 折扣计算总收益（向下取整以防小数）
	const int32 TotalGold = FMath::FloorToInt(static_cast<float>(SellPriceUnit) * SellPriceMultiplier) * Count;
	if (TotalGold <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Shop] SellItem: 计算出售收益为0"));
		return;
	}

	// 4) 移除物品 → 加金币
	if (!Inventory->RemoveItem(ItemID, Count))
	{
		return;
	}
	Inventory->AddGold(TotalGold);

	UE_LOG(LogTemp, Log, TEXT("[Shop] SellItem: %s 出售 %s x%d 获得 %d 金币"),
		*Seller->GetName(), *ItemID.ToString(), Count, TotalGold);
}

// ===== 客户端通知 =====

void UPP_ShopComponent::Client_OnShopResult_Implementation(const FText& Message)
{
	OnShopResult.Broadcast(Message);
}
