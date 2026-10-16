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

/** 任务列表变更事件：QuestComponent -> UI（接取/放弃/提交/进度更新/OnRep 后广播） */
USTRUCT(BlueprintType)
struct FPP_QuestsUpdatedEvent
{
	GENERATED_BODY()

	/** 任务归属玩家（UI 据此过滤，只处理本地玩家的变更） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;
};

/** 库存变更事件：InventoryComponent -> UI（增删/堆叠变化/OnRep 后广播） */
USTRUCT(BlueprintType)
struct FPP_InventoryUpdatedEvent
{
	GENERATED_BODY()

	/** 库存归属玩家（UI 据此过滤，只处理本地玩家的变更） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;
};

/** 装备变更事件：InventoryComponent -> UI（穿戴/脱下/OnRep 后广播） */
USTRUCT(BlueprintType)
struct FPP_EquipmentUpdatedEvent
{
	GENERATED_BODY()

	/** 库存归属玩家（UI 据此过滤，只处理本地玩家的变更） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;
};

/** 金币变更事件：InventoryComponent -> UI（增加/扣减/OnRep 后广播） */
USTRUCT(BlueprintType)
struct FPP_GoldUpdatedEvent
{
	GENERATED_BODY()

	/** 库存归属玩家（UI 据此过滤，只处理本地玩家的变更） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Player = nullptr;

	/** 变更后的金币数量 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 NewGold = 0;
};

/** 商店操作结果事件：ShopComponent -> UI（购买/出售失败原因提示，客户端本地广播） */
USTRUCT(BlueprintType)
struct FPP_ShopResultEvent
{
	GENERATED_BODY()

	/** 结果提示文本（金币不足、库存不足等） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FText Message;
};

/** 购买完成事件：ShopComponent -> UI（库存扣减后刷新商店列表；进程内广播，standalone 下可送达 UI） */
USTRUCT(BlueprintType)
struct FPP_PurchaseCompletedEvent
{
	GENERATED_BODY()

	/** 购买者的 PlayerController（UI 据此过滤） */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	TObjectPtr<APlayerController> Buyer = nullptr;

	/** 购买的物品ID */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	FName ItemID;

	/** 购买数量 */
	UPROPERTY(BlueprintReadOnly, Category = "Event")
	int32 Count = 0;
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


