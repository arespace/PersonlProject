// 耐力系统组件 — 消耗 / 恢复 / 恢复延迟 / 冲刺与翻滚扣费

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayEffectTypes.h"
#include "PP_StaminaComponent.generated.h"

class UGameplayEffect;
class APP_BaseCharacter;

/**
 * 玩家耐力组件。
 *
 * 归属 PersonalProjectCore：依赖只有 APP_BaseCharacter、UPP_AttributeSet、PPTags（均在本模块）
 * 以及 ALS 的 Gait / LocomotionAction（Core 已公开依赖 ALS），
 * 因此不会引入 Core → PersonalProject 的反向依赖。
 *
 * 耐力数值本身仍存放在 UPP_AttributeSet 上：本组件不持有 AttributeSet 指针，
 * 只通过 ASC 的属性变化委托订阅"耐力下降"，消耗与恢复全部由 GE 执行。
 *
 * 更新时机：由 APP_PlayerCharacter::Tick 在 Super::Tick 之后手动调用 UpdateStamina()，
 * 保持迁移前的帧内顺序（ALS → 相机/锁定 → 耐力）。
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECTCORE_API UPP_StaminaComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_StaminaComponent();

	/** 绑定 ASC 的耐力属性变化委托；PossessedBy 与 OnRep_PlayerState 都会调用，内部先 RemoveAll 再 Add */
	void BindToAbilitySystem();

	/** 施加常驻的耐力回复 / 奔跑消耗 GE（Infinite 周期型，是否生效由 OngoingTagRequirements 决定） */
	void ApplyPassiveEffects();

	/** 由角色 Tick 在 Super::Tick 之后调用：维护冲刺状态 tag + 检测翻滚起始扣费 */
	void UpdateStamina();

	/** 当前耐力是否足够支付 Amount（客户端用复制过来的值预检） */
	UFUNCTION(BlueprintPure, Category = "PP|Stamina")
	bool HasEnoughStamina(float Amount) const;

	/** 通过 StaminaCostEffect 扣除指定体力（SetByCaller 传 -Amount） */
	UFUNCTION(BlueprintCallable, Category = "PP|Stamina")
	void ApplyStaminaCost(float Amount);

	// ===== 配置（原在 APP_PlayerCharacter 上，迁移后可在 BP_PP_PlayerCharacter 的 StaminaComponent 上覆盖） =====

	/**
	 * 耐力回复 GE（Infinite + Period 0.1s，Modifier = StaminaRegenRate × 0.1）。
	 * OngoingTagRequirements 禁止 PPTags.Status.Sprinting / PPTags.Status.StaminaRegenBlocked，
	 * 实现"奔跑中不回体、消耗后 0.8s 内不回体"，无需 C++ 手写计时器。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	/**
	 * 奔跑消耗 GE（Infinite + Period 0.1s，Modifier = -0.8）。
	 * OngoingTagRequirements 要求 PPTags.Status.Sprinting，只有真的在冲刺时才跳。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaSprintDrainEffect;

	/**
	 * 耐力回复延迟 GE（Duration 型，固定 0.8s，GrantedTag = PPTags.Status.StaminaRegenBlocked）。
	 * 任何一次耐力下降都会刷新它，从而抑制常驻的 GE_StaminaRegen。
	 * 留空则耐力回复没有延迟窗口（仅用于分批调试）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffect;

	/** 翻滚体力消耗值（体力不足时不触发翻滚） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina", Meta = (ClampMin = 0))
	float RollStaminaCost{20.f};

	/**
	 * 翻滚的体力扣除 GE（Instant，SetByCaller = PPTags.SetByCaller.StaminaCost）。
	 * 和技能共用同一个资产（GE_Cost_Stamina）。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

private:
	/** 耐力下降时施加回复延迟 GE（覆盖奔跑 / 翻滚 / 技能所有消耗来源） */
	void OnStaminaChanged(const FOnAttributeChangeData& AttributeChangeData);

	/** 服务端每帧轮询 ALS Gait / LocomotionAction，维护 PP.Status.Sprinting loose tag */
	void UpdateSprintStateTag();

	/** 服务端检测 ALS 进入翻滚瞬间，扣除翻滚体力 */
	void UpdateRollStaminaCost();

	APP_BaseCharacter* GetOwnerCharacter() const;

	/** 上一帧是否处于翻滚状态（用于检测翻滚起始瞬间） */
	bool bWasRolling{false};
};
