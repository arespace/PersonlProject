// 存档系统 — 存档数据结构
// 只放纯数据，不放逻辑；所有字段必须带 UPROPERTY，才能被 USaveGame 序列化。
// 版本升级时新增字段一律给默认值，旧档缺失字段读出来即默认值。

#pragma once

#include "CoreMinimal.h"
#include "Data/PP_ItemData.h"
#include "Data/PP_QuestData.h"
#include "PP_SaveTypes.generated.h"

/**
 * 角色进度存档。
 * 属性只存 base 值：装备 GE 的 Additive Modifier 不落盘，读档时由装备恢复流程重新施加，
 * 避免"先写属性再恢复装备"导致加成叠两次。
 */
USTRUCT()
struct FPP_SaveAttributeData
{
	GENERATED_BODY()

	UPROPERTY() float Health = 0.f;
	UPROPERTY() float MaxHealth = 0.f;
	UPROPERTY() float Mana = 0.f;
	UPROPERTY() float MaxMana = 0.f;
	UPROPERTY() float Stamina = 0.f;
	UPROPERTY() float MaxStamina = 0.f;
	UPROPERTY() float Attack = 0.f;
	UPROPERTY() float Defense = 0.f;
	UPROPERTY() float StaminaRegenRate = 0.f;
};

USTRUCT()
struct FPP_SaveCharacterData
{
	GENERATED_BODY()

	/** 角色 ID（DT_Characters / GameMode 能力映射的行名） */
	UPROPERTY() FName CharacterId;

	/** 九项属性的 base 值 */
	UPROPERTY() FPP_SaveAttributeData BaseAttributes;
};

/**
 * 背包与装备存档。
 * 装备只存 ItemID + Slot：EffectHandle 是运行时句柄，读档时按 DT_Items.EquipEffectClass 重新施加并重建。
 */
USTRUCT()
struct FPP_SaveInventoryData
{
	GENERATED_BODY()

	UPROPERTY() TArray<FPP_ItemInstance> Items;

	UPROPERTY() TArray<FPP_EquippedItem> Equipped;

	UPROPERTY() int32 Gold = 0;
};

/**
 * 任务进度存档。
 * 只存任务 ID 与运行时计数，目标结构（类型/描述/需求数量）读档时以 DT_Task 为准重新校正。
 */
USTRUCT()
struct FPP_SaveQuestData
{
	GENERATED_BODY()

	UPROPERTY() TArray<FPP_ActiveQuest> ActiveQuests;

	UPROPERTY() TArray<FName> CompletedQuestIDs;
};

/** 篝火检查点存档：只决定读档后的首次生成位置，不参与复活流程 */
USTRUCT()
struct FPP_SaveCheckpointData
{
	GENERATED_BODY()

	UPROPERTY() bool bHasCheckpoint = false;

	UPROPERTY() FTransform Transform = FTransform::Identity;

	/** 仅用于排查，加载时不依赖它 */
	UPROPERTY() FName BonfireActorName;
};

/** 单个采集点的剩余次数（按关卡 Actor 名定位） */
USTRUCT()
struct FPP_SaveGatherableState
{
	GENERATED_BODY()

	UPROPERTY() FName ActorName;

	/** 仅用于排查 */
	UPROPERTY() FName GatherableID;

	UPROPERTY() int32 RemainingGatherTimes = 0;
};

/** Boss 击败标记：按 BossName 记录，Boss Actor 被销毁后仍能保留 */
USTRUCT()
struct FPP_SaveBossState
{
	GENERATED_BODY()

	UPROPERTY() FName BossName;

	/** 仅用于排查 */
	UPROPERTY() FName ActorName;

	UPROPERTY() bool bDefeated = false;
};

/** 玩家死亡掉落的魂 */
USTRUCT()
struct FPP_SaveSoulState
{
	GENERATED_BODY()

	UPROPERTY() int32 Amount = 0;

	UPROPERTY() FVector Location = FVector::ZeroVector;
};

/** 世界状态存档：只记录与默认状态不同的部分 */
USTRUCT()
struct FPP_SaveWorldData
{
	GENERATED_BODY()

	UPROPERTY() TArray<FPP_SaveGatherableState> Gatherables;

	UPROPERTY() TArray<FPP_SaveBossState> Bosses;

	UPROPERTY() TArray<FPP_SaveSoulState> Souls;
};
