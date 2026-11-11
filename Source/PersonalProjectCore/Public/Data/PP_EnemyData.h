// P5 配置数据化 — 敌人 / Boss 配置行结构
// 作为 DataTable (DT_Enemies) 的行结构使用，行名 = EnemyID

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PP_EnemyData.generated.h"

class UGameplayAbility;

/**
 * 敌人数据行
 *
 * 覆盖 APP_EnemyCharacter / APP_BossCharacter 上原本只能逐个蓝图手填的数值，
 * 由 UPP_DataManager 统一查询，缺行时调用方沿用蓝图配置（行为不变）。
 */
USTRUCT(BlueprintType)
struct FPP_EnemyInfo : public FTableRowBase
{
	GENERATED_BODY()

	/** 敌人唯一标识（= 行名），须与敌人蓝图上的 EnemyID 一致 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy")
	FName EnemyID;

	// ===== AI 战斗参数 =====

	/** MoveTo 接受半径（cm）：走到距目标该距离内即视为到位 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|AI", meta = (ClampMin = "0.0"))
	float AcceptanceRadius{500.f};

	/** 索敌轮次前随机延迟下限（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|AI", meta = (ClampMin = "0.0"))
	float MinAttackDelay{0.1f};

	/** 索敌轮次前随机延迟上限（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|AI", meta = (ClampMin = "0.0"))
	float MaxAttackDelay{0.5f};

	/** 攻击后冷却下限（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|AI", meta = (ClampMin = "0.0"))
	float AttackCooldownMin{1.5f};

	/** 攻击后冷却上限（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|AI", meta = (ClampMin = "0.0"))
	float AttackCooldownMax{2.5f};

	// ===== 死亡与掉落 =====

	/** 死亡时直接发放给击杀者的金币数量（Boss 行填 500，再乘 BossGoldMultiplier） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Loot", meta = (ClampMin = "0"))
	int32 GoldDropAmount{10};

	/** 死亡时直接放入击杀者背包的物品 ID（留空表示不发放物品） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Loot")
	FName DropItemID;

	/** 上述物品的发放数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Loot", meta = (ClampMin = "0"))
	int32 DropItemCount{1};

	/** 死亡后是否复活（对应 GA_PP_Death 的 ShouldRespawn） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Death")
	bool bShouldRespawn{false};

	// ===== Boss 专用（非 Boss 行留默认值即可） =====

	/** Boss 显示名（出现在顶部血条） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Boss")
	FName BossName;

	/** 血量降到哪些百分比时切阶段（升序，0~1）；留空则沿用蓝图配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Boss")
	TArray<float> BossPhaseThresholds;

	/** 阶段切换无敌时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Boss", meta = (ClampMin = "0.0"))
	float PhaseTransitionInvulnerableTime{1.5f};

	/** 死亡金币倍率（相对 GoldDropAmount） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Boss", meta = (ClampMin = "1.0"))
	float BossGoldMultiplier{10.f};

	/** 场地激活后授予的索敌 GA（留空则沿用蓝图配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Enemy|Boss")
	TSubclassOf<UGameplayAbility> SearchForTargetAbilityClass;
};
