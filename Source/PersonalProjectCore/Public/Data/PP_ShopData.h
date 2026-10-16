// 商店系统 — 商店数据结构定义
// 包含商店商品行（DataTable）与商店配置结构

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PP_ShopData.generated.h"

/**
 * 商店商品行 — DataTable 行结构
 * 单条商品记录，关联到 FPP_ItemInfo 物品数据表中的具体物品
 * 支持库存数量限制与价格覆盖
 */
USTRUCT(BlueprintType)
struct FPP_ShopItem : public FTableRowBase
{
	GENERATED_BODY()

	/** 物品唯一标识符，对应 FPP_ItemInfo::ItemID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	FName ItemID;

	/** 库存数量，-1 表示无限供应 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop")
	int32 Stock = -1;

	/** 价格覆盖，0 表示使用 ItemData 中配置的 BuyPrice */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop", meta = (ClampMin = "0"))
	int32 PriceOverride = 0;
};

