// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectPtr.h"
#include "PP_EventTypes.generated.h"

class APlayerController;

/**
 * 事件总线使用的事件类型集合。
 * 所有跨系统通信通过事件结构体传递，避免直接依赖。
 * 新增事件时在此文件追加 USTRUCT，并在 EventBus 中使用 Subscribe/Broadcast。
 */

/** 任务完成事件：QuestComponent -> HUD/UI/其他监听者 */
USTRUCT(BlueprintType)
struct FPP_QuestCompletedEvent
{
	GENERATED_BODY()

	/** 完成任务的玩家 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;

	/** 完成的任务 ID */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName QuestID;

	/** 任务显示名（发布方组装，订阅方无需查表） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FString QuestName;
};

/** 采集完成事件：GatherableActor -> Quest/UI */
USTRUCT(BlueprintType)
struct FPP_GatherEvent
{
	GENERATED_BODY()

	/** 执行采集的玩家 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Gatherer = nullptr;

	/** 采集到的物品 ID */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName ItemID;

	/** 采集数量 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 Count = 0;
};

/** 物品拾取/添加事件：Inventory -> Quest/UI */
USTRUCT(BlueprintType)
struct FPP_ItemPickedUpEvent
{
	GENERATED_BODY()

	/** 获得物品的玩家 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;

	/** 物品 ID */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName ItemID;

	/** 添加后的物品数量 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 NewCount = 0;
};

/** 对话完成事件：NPC -> Quest */
USTRUCT(BlueprintType)
struct FPP_DialogueCompletedEvent
{
	GENERATED_BODY()

	/** 参与对话的玩家 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;

	/** NPC 标识 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName NPCID;
};

/** 玩家死亡事件：Character -> UI/系统 */
USTRUCT(BlueprintType)
struct FPP_PlayerDeathEvent
{
	GENERATED_BODY()

	/** 死亡的玩家 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;
};
