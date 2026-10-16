// 库存系统 — 物品数据结构定义
// 包含物品信息行（DataTable）、物品实例、已装备物品记录

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "GameplayEffectTypes.h"
#include "Inventory/Data/PP_ItemTypes.h"
#include "PP_ItemData.generated.h"

class UGameplayEffect;
class UTexture2D;

/**
 * 物品数据行 — DataTable 行结构
 * 定义物品的静态配置信息，通过 DataTable 进行数据驱动配置
 */
USTRUCT(BlueprintType)
struct FPP_ItemInfo : public FTableRowBase
{
	GENERATED_BODY()

	/** 物品唯一标识符 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	/** 物品显示名称 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText DisplayName;

	/** 物品描述文字 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FText Description;

	/** 物品类型（材料/装备/消耗品/任务物品/货币） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	EPP_ItemType ItemType = EPP_ItemType::Material;

	/** 物品图标（软引用，按需加载） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	TSoftObjectPtr<UTexture2D> Icon;

	/** 最大堆叠数量，99表示可堆叠，设置为1表示不可堆叠（如装备） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 MaxStackCount = 99;

	/** 购买价格 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy")
	int32 BuyPrice = 0;

	/** 出售价格 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Economy")
	int32 SellPrice = 0;

	/** 物品标签（用于分类、过滤和任务系统匹配） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FGameplayTagContainer ItemTags;

	// ===== 装备专用字段 =====

	/** 装备槽位，仅 Equipment 类型有效 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment")
	EPP_EquipSlot EquipSlot = EPP_EquipSlot::None;

	/** 穿戴时施加的 GameplayEffect 类（提供固定属性加成） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment")
	TSubclassOf<UGameplayEffect> EquipEffectClass;

	/** 物品稀有度，用于UI颜色分级 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Equipment")
	EPP_ItemRarity Rarity = EPP_ItemRarity::Common;

	/** 消耗品使用时施加的 GameplayEffect（回血/回蓝等） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item|Consumable")
	TSubclassOf<UGameplayEffect> ConsumeEffectClass;
};

/**
 * 库存中的物品实例
 * 表示背包中的一条物品记录，包含物品ID和数量
 */
USTRUCT(BlueprintType)
struct FPP_ItemInstance
{
	GENERATED_BODY()

	FPP_ItemInstance() : Count(0) {}
	FPP_ItemInstance(FName InItemID, int32 InCount) : ItemID(InItemID), Count(InCount) {}

	/** 物品唯一标识符，对应 FPP_ItemInfo::ItemID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	FName ItemID;

	/** 当前堆叠数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item")
	int32 Count;

	bool operator==(const FPP_ItemInstance& Other) const
	{
		return ItemID == Other.ItemID;
	}
};

/**
 * 已装备物品记录
 * 记录装备信息和对应的 GE 句柄，用于脱下时精确移除效果
 */
USTRUCT(BlueprintType)
struct FPP_EquippedItem
{
	GENERATED_BODY()

	FPP_EquippedItem() : Slot(EPP_EquipSlot::None) {}

	/** 已装备的物品ID */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	FName ItemID;

	/** 该装备所在槽位 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Equipment")
	EPP_EquipSlot Slot = EPP_EquipSlot::None;

	/** GE 句柄，脱下装备时通过此句柄移除对应的 GameplayEffect */
	UPROPERTY(NotReplicated)
	FActiveGameplayEffectHandle EffectHandle;
};
