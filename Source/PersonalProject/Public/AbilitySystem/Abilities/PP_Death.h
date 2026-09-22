// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "GameplayEffect.h"
#include "PP_Death.generated.h"

class UAnimMontage;

/**
 * 通用死亡/复活技能（GA_PP_Death）的 C++ 实现，玩家与敌人共用：
 * 播死亡蒙太奇 → 施加 GE_Death（授予 PPTags.Status.Dead）→ 蒙太奇结束后
 * 敌人按 bShouldRespawn 决定复活或销毁尸体；非敌人（玩家）直接复活。
 */
UCLASS()
class PERSONALPROJECT_API UPP_Death : public UPP_GameplayAbility
{
	GENERATED_BODY()

public:
	UPP_Death();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

protected:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> DeathEffectClass;

private:
	// 缓存激活参数，供蒙太奇结束回调里调用 EndAbility
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	void ApplyDeathEffect();
	void Respawn();

	UFUNCTION()
	void OnDeathMontageFinished();
};
