// 任务系统 — 任务数据结构定义
// 包含任务目标类型枚举、目标结构体、奖励结构体、任务信息行结构、运行时活跃任务结构

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Inventory/Data/PP_ItemData.h"
#include "PP_QuestData.generated.h"

/**
 * 任务目标类型枚举
 * 定义任务系统支持的目标追踪类型
 */
UENUM(BlueprintType)
enum class EPP_QuestObjectiveType : uint8
{
	/** 击杀指定敌人 */
	KillEnemy		UMETA(DisplayName = "KillEnemy"),
	/** 采集指定物品 */
	GatherItem		UMETA(DisplayName = "GatherItem"),
	/** 从商店购买指定物品 */
	PurchaseItem	UMETA(DisplayName = "PurchaseItem"),
	/** 发送聊天消息 */
	SendChat		UMETA(DisplayName = "SendChat"),
	/** 与指定NPC交谈 */
	TalkToNPC		UMETA(DisplayName = "TalkToNPC"),
	/** 自定义（Tag触发） */
	Custom			UMETA(DisplayName = "Custom")
};

/**
 * 任务目标结构体
 * 描述任务的单个目标条件
 */
USTRUCT(BlueprintType)
struct FPP_QuestObjective
{
	GENERATED_BODY()

	FPP_QuestObjective()
		: Type(EPP_QuestObjectiveType::Custom)
		, RequiredCount(1)
		, CurrentCount(0)
		, bCompleted(false)
		, bSharedInTeam(false)
	{}

	/** 目标类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	EPP_QuestObjectiveType Type;

	/** 目标描述文本（UI显示用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	/** 目标ID（敌人ID/物品ID/NPC ID 等，根据Type解释） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName TargetID;

	/** 需要完成的数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 RequiredCount;

	/** 当前完成数量（运行时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 CurrentCount;

	/** 是否已完成（运行时） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bCompleted;

	/** 是否组队共享进度 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bSharedInTeam;
};

/**
 * 任务奖励结构体
 * 定义任务完成后发放的奖励
 */
USTRUCT(BlueprintType)
struct FPP_QuestReward
{
	GENERATED_BODY()

	FPP_QuestReward()
		: GoldReward(0)
		, ExpReward(0)
	{}

	/** 金币奖励 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 GoldReward;

	/** 物品奖励列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FPP_ItemInstance> ItemRewards;

	/** 经验奖励 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	int32 ExpReward;
};

/**
 * 任务信息行 — DataTable 行结构
 * 定义一个任务的完整配置（ID、名称、目标、奖励、前置等）
 */
USTRUCT(BlueprintType)
struct FPP_QuestInfo : public FTableRowBase
{
	GENERATED_BODY()

	FPP_QuestInfo()
		: bIsMainQuest(false)
	{}

	/** 任务唯一标识符 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestID;

	/** 任务显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText QuestName;

	/** 任务描述 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FText Description;

	/** 是否为主线任务（true=主线, false=支线） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	bool bIsMainQuest;

	/** 前置任务ID列表（主线任务串联用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FName> PrerequisiteQuestIDs;

	/** 主线完成后自动解锁的下一个任务ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName NextMainQuestID;

	/** 任务目标列表 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FPP_QuestObjective> Objectives;

	/** 任务奖励 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FPP_QuestReward Reward;

	/** 任务接取NPC的ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName StartNPCID;

	/** 任务提交NPC的ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName TurnInNPCID;
};

/**
 * 运行时活跃任务结构体
 * 记录玩家正在进行的任务当前状态
 */
USTRUCT(BlueprintType)
struct FPP_ActiveQuest
{
	GENERATED_BODY()

	FPP_ActiveQuest() {}

	/** 任务ID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	FName QuestID;

	/** 当前各目标进度（从 QuestInfo.Objectives 复制并运行时更新） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Quest")
	TArray<FPP_QuestObjective> Objectives;
};
