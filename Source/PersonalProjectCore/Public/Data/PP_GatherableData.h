// P5 配置数据化 — 采集点配置行结构
// 作为 DataTable (DT_Gatherables) 的行结构使用，行名 = GatherableID

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PP_GatherableData.generated.h"

/**
 * 采集点数据行
 *
 * 覆盖 APP_GatherableActor 上原本只能逐实例手填的配置，
 * 由 UPP_DataManager 统一查询；缺行或字段为 0/空时调用方沿用蓝图现值（行为不变）。
 */
USTRUCT(BlueprintType)
struct FPP_GatherableInfo : public FTableRowBase
{
	GENERATED_BODY()

	/** 采集点唯一标识（= 行名），须与采集物蓝图上的 GatherableID 一致 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable")
	FName GatherableID;

	// ===== 采集配置 =====

	/** 采集产出的物品 ID（= DT_Items 行名）；留空表示沿用蓝图上的 ItemID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable")
	FName ItemID;

	/** 每次采集产出数量；0 表示沿用蓝图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable", meta = (ClampMin = "0"))
	int32 GatherCount{0};

	/** 单次采集耗时（秒）；0 表示沿用蓝图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable", meta = (ClampMin = "0.0"))
	float GatherTime{0.f};

	/** 耗尽前可被采集的次数；0 表示沿用蓝图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable", meta = (ClampMin = "0"))
	int32 MaxGatherTimes{0};

	/** 耗尽后重生等待时间（秒）；0 表示沿用蓝图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable", meta = (ClampMin = "0.0"))
	float RespawnTime{0.f};

	/** 交互范围球半径（cm）；0 表示沿用蓝图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Gatherable", meta = (ClampMin = "0.0"))
	float InteractionRadius{0.f};
};
