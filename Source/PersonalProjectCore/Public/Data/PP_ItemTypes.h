// 库存系统 — 物品类型与装备槽位枚举定义

#pragma once

#include "CoreMinimal.h"
#include "PP_ItemTypes.generated.h"

/**
 * 物品类型枚举
 * 用于区分物品的基本分类，影响物品的使用方式和存储逻辑
 */
UENUM(BlueprintType)
enum class EPP_ItemType : uint8
{
	Material    UMETA(DisplayName = "材料"),
	Equipment   UMETA(DisplayName = "装备"),
	Consumable  UMETA(DisplayName = "消耗品"),
	QuestItem   UMETA(DisplayName = "任务物品"),
	Currency    UMETA(DisplayName = "货币")
};

/**
 * 装备槽位枚举
 * 定义角色身上可穿戴装备的位置，每个槽位至多一件装备
 */
UENUM(BlueprintType)
enum class EPP_EquipSlot : uint8
{
	None        UMETA(DisplayName = "无"),

	// 武器槽
	MainHand    UMETA(DisplayName = "主手"),
	OffHand     UMETA(DisplayName = "副手"),

	// 防具槽
	Head        UMETA(DisplayName = "头部"),
	Chest       UMETA(DisplayName = "胸部"),
	Legs        UMETA(DisplayName = "腿部"),
	Feet        UMETA(DisplayName = "脚部"),
	Hands       UMETA(DisplayName = "手部"),

	// 饰品槽
	Ring        UMETA(DisplayName = "戒指")
};

/**
 * 物品稀有度枚举
 * 决定物品边框颜色和掉落权重
 */
UENUM(BlueprintType)
enum class EPP_ItemRarity : uint8
{
	Common      UMETA(DisplayName = "普通"),    // 白色 #FFFFFF
	Uncommon    UMETA(DisplayName = "优秀"),    // 绿色 #00FF00
	Rare        UMETA(DisplayName = "稀有"),    // 蓝色 #0080FF
	Epic        UMETA(DisplayName = "史诗"),    // 紫色 #A020F0
	Legendary   UMETA(DisplayName = "传说")     // 金色 #FFD700
};
