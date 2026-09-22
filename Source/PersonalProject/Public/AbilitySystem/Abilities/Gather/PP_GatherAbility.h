// 采集系统 — GAS采集能力
// 通过GAS驱动的采集行为：施加State.Gathering Tag、定时完成、可被打断

#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "PP_GatherAbility.generated.h"

class APP_GatherableActor;

/**
 * 采集能力（UGameplayAbility）
 *
 * 职责：
 *   1. 激活时施加 State.Gathering Tag 标记角色进入采集状态
 *   2. 使用 FTimerHandle 计时（与采集物的 GatherTime 匹配）
 *   3. 计时完成后触发采集物的 CompleteGather
 *   4. 松开按键 / 受到伤害（State.Gathering 被移除）/ 角色移动时自动取消
 *   5. 通过 AbilityTags + BlockAbilitiesWithTag 阻止采集期间使用其他能力
 *
 * 配置方式：在蓝图中或 GiveAbility 时设定 AbilityTags、ActivationOwnedTags 等
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API UPP_GatherAbility : public UGameplayAbility
{
	GENERATED_BODY()

public:
	UPP_GatherAbility();

	//~ Begin UGameplayAbility Interface
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		const FGameplayEventData* TriggerEventData) override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayAbilityActivationInfo ActivationInfo,
		bool bReplicateEndAbility, bool bWasCancelled) override;

	virtual bool CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags = nullptr,
		const FGameplayTagContainer* TargetTags = nullptr,
		FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	//~ End UGameplayAbility Interface

	/** 设置当前正在采集的目标Actor（在激活前由外部设置） */
	UFUNCTION(BlueprintCallable, Category = "Gather")
	void SetGatherTarget(APP_GatherableActor* InTarget);

protected:
	/** 受到伤害Tag时取消能力（需在蓝图或默认对象中配置） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gather|Config")
	FGameplayTag CancelOnDamageTag;

	/** 采集动画蒙太奇（可选，不配置则无动画） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gather|Config")
	TObjectPtr<UAnimMontage> GatherMontage;

	/** 当前采集目标 */
	UPROPERTY(BlueprintReadOnly, Category = "Gather")
	TObjectPtr<APP_GatherableActor> GatherTarget;

private:
	/** 采集计时句柄 */
	FTimerHandle GatherTimerHandle;

	/** 采集计时完成回调 */
	void OnGatherTimerCompleted();

	/** 检测Tag变化来感知被打断 */
	void OnGatheringTagRemoved(const FGameplayTag Tag, int32 NewCount);

	/** 缓存能力参数用于End时访问 */
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo;
	FGameplayAbilityActivationInfo CachedActivationInfo;
};
