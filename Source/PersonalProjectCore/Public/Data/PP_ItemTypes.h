// 库存系统 — 物品类型与装备槽位枚举定义

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "PP_ItemTypes.generated.h"

class UGameplayAbility;
class UStaticMesh;

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
 * 武器外观配置
 * 各武器模型枢轴位置不同，必须逐把配置挂载变换
 */
USTRUCT(BlueprintType)
struct FPP_WeaponVisual
{
	GENERATED_BODY()

	/** 武器静态网格（软引用，装备时按需加载） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	TSoftObjectPtr<UStaticMesh> Mesh;

	/** 挂载到角色骨骼的插槽名（NAME_None 时由代码使用默认插槽） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FName AttachSocket = NAME_None;

	/** 相对挂载点的位移 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FVector RelativeLocation = FVector::ZeroVector;

	/** 相对挂载点的旋转 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FRotator RelativeRotation = FRotator::ZeroRotator;

	/** 相对挂载点的缩放 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon|Visual")
	FVector RelativeScale = FVector::OneVector;
};

/**
 * 武器完整配置
 * 内嵌在物品数据行中，用于取代独立的武器数据表
 */
USTRUCT(BlueprintType)
struct FPP_WeaponConfig
{
	GENERATED_BODY()

	/** 装备该武器时需要授予的 GameplayAbility 类（空则不授予 GA） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	TSubclassOf<UGameplayAbility> WeaponAbilityClass;

	/** 网格与挂载变换 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	FPP_WeaponVisual Visual;

	/** 是否允许装备到副手 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Weapon")
	bool bCanUseOffHand = false;
};
