// 商店系统 — 商店只读查询接口
// 解耦 UI 对 UPP_ShopComponent 具体类的直接持有：
// UI 通过本接口读取商品数据，通过 EventBus 的 FPP_ShopResultEvent 感知操作结果。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_ShopData.h"
#include "PP_ShopQuery.generated.h"

UINTERFACE(BlueprintType)
class UPP_ShopQuery : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_ShopQuery — 商店数据只读查询能力接口
 * 实现方：UPP_ShopComponent（挂载在商人 NPC 上）
 * 调用方：UI 模块（商店面板、商品条目、数量选择弹窗）
 *
 * 约定：本接口只读，不提供任何写操作；购买/出售走 IPP_ShopActions。
 * 物品定义（名称/图标/基础价）属全局数据，走 DataManager 查询，不在此接口内。
 */
class PERSONALPROJECTCORE_API IPP_ShopQuery
{
	GENERATED_BODY()

public:
	/** 查询当前商店的所有商品列表 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Query")
	TArray<FPP_ShopItem> QueryShopItems();

	/** 查询商品的实际购买价（考虑 PriceOverride） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Query")
	int32 QueryItemBuyPrice(FName ItemID);

	/** 查询出售折扣系数 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Query")
	float QuerySellPriceMultiplier();
};
