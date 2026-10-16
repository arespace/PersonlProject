// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "PP_MontageEventAbility.generated.h"

class UAnimMontage;

/**
 * 蒙太奇事件技能基类：收敛 Primary/Secondary/Tertiary 共用的激活流程。
 * 流程：施加 BlockHitReact GE（施法期间免受击打断）→ 播放蒙太奇 →
 *       监听蒙太奇里 AnimNotify 发出的技能事件 → 子类响应 → 结束时移除 GE。
 *
 * 子类只需提供：要等待的事件 Tag（GetEventTagToWait）、事件到达后的响应（OnAbilityEventReceived），
 * FlipFlop 类技能可 override GetMontageToPlay 自行选择蒙太奇。
 */
UCLASS(Abstract)
class PERSONALPROJECT_API UPP_MontageEventAbility : public UPP_GameplayAbility
{
	GENERATED_BODY()

public:
	UPP_MontageEventAbility();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 施法期间阻止受击反应（对应 GE_BlockHitReact）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> BlockHitReactEffectClass;

	// BlockHitReact GE 施加等级（Primary 用 0，Secondary/Tertiary 用 1）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float BlockHitReactLevel = 1.0f;

	// 施法蒙太奇（对应 CreatePlayMontageAndWaitProxy 的 MontageToPlay）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> AbilityMontage;

	// 能力结束时是否停止蒙太奇（Primary 为 true，Secondary/Tertiary 为 false）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	bool bStopMontageWhenAbilityEnds = false;

	// 缓存激活参数，供蒙太奇结束回调里调用 EndAbility / 子类 CommitAbility
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	// 本次激活要播放的蒙太奇（默认取 AbilityMontage，FlipFlop 技能 override）
	virtual UAnimMontage* GetMontageToPlay();

	// 蒙太奇里 AnimNotify 发出的事件 Tag（每个技能不同，子类必须 override）
	virtual FGameplayTag GetEventTagToWait() const { return FGameplayTag::EmptyTag; }

	// 事件到达后的技能响应（Commit / 命中 / 生成投射物等由子类实现，子类必须 override）
	virtual void OnAbilityEventReceived(FGameplayEventData Payload) {}

private:
	UFUNCTION()
	void OnAbilityMontageFinished();

	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);
};
