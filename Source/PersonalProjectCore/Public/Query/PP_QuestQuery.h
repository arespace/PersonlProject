// 任务系统 — 任务只读查询接口
// 解耦 UI 对 UPP_QuestComponent 具体类的直接持有：
// UI 通过本接口读取任务数据，通过 EventBus 的 FPP_QuestsUpdatedEvent 感知变更。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_QuestData.h"
#include "PP_QuestQuery.generated.h"

UINTERFACE(BlueprintType)
class UPP_QuestQuery : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_QuestQuery — 任务数据只读查询能力接口
 * 实现方：UPP_QuestComponent（挂载在 APP_PlayerState 上）
 * 调用方：UI 模块（任务追踪器、任务日志等）
 *
 * 约定：本接口只读，不提供任何写操作；写操作仍由任务系统内部触发。
 */
class PERSONALPROJECTCORE_API IPP_QuestQuery
{
	GENERATED_BODY()

public:
	/** 查询当前活跃任务列表 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Query")
	TArray<FPP_ActiveQuest> QueryActiveQuests();

	/** 查询已完成任务ID列表 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Query")
	TArray<FName> QueryCompletedQuests();

	/** 查询指定任务配置信息；未找到返回 false */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Quest|Query")
	bool QueryQuestInfo(FName QuestID, FPP_QuestInfo& OutInfo);
};
