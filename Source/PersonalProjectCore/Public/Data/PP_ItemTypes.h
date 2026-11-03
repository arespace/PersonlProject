// 库存系统 — 物品类型与装备槽位枚举定义

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
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

/**
 * 武器系统专用装备槽枚举
 * 与现有 EPP_EquipSlot 并行，聚焦 GAS Ability 切换场景
 */
UENUM(BlueprintType)
enum class EPP_EquipmentSlotType : uint8
{
	PrimaryWeapon    UMETA(DisplayName = "主手武器"),
	SecondaryWeapon  UMETA(DisplayName = "副手武器"),
	Helmet           UMETA(DisplayName = "头盔"),
	Accessory        UMETA(DisplayName = "饰品")
};

/**
 * 武器装备槽数据结构
 * 记录一个槽位上当前装备的物品ID
 */
USTRUCT(BlueprintType)
struct FPP_EquipmentSlot
{
	GENERATED_BODY()

	FPP_EquipmentSlot() : SlotType(EPP_EquipmentSlotType::PrimaryWeapon) {}
	FPP_EquipmentSlot(EPP_EquipmentSlotType InSlotType) : SlotType(InSlotType) {}

	/** 槽位类型 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EPP_EquipmentSlotType SlotType;

	/** 当前装备的物品ID（NAME_None 表示空槽） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName ItemID = NAME_None;
};

/**
 * 武器数据表行结构
 * 每把武器关联一个 GameplayAbility 类，装备/卸下时 PlayerCharacter 据此切换 GA
 */
USTRUCT(BlueprintType)
struct FPP_WeaponData : public FTableRowBase
{
	GENERATED_BODY()

	/** 物品ID（与 FPP_ItemInfo::ItemID 对应，同时作为 DataTable RowName） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FName ItemID;

	/** 装备该武器时需要授予的 GameplayAbility 类（空则不授予 GA） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGameplayAbility> WeaponAbilityClass;

	/** Ability Tags（用于 GA 识别 / 匹配，可选填） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TArray<FGameplayTag> AbilityTags;

	/** 伤害倍率（由 GameplayEffect 消费，这里仅存表供后续扩展） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon", meta = (ClampMin = "0.0"))
	float DamageMultiplier = 1.f;
};
