
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
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Abilities")
	TObjectPtr<UAnimMontage> DeathMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Abilities")
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

	// Ragdoll 接管死亡表现后，蒙太奇回调（尤其服务端）不可靠。
	// 玩家复活已移到 APP_BaseCharacter::ServerRespawn（HandleDeath 启动的服务端计时器）；
	// 此守卫防止 GA 侧回调与角色侧复活重复执行。
	bool bRespawnHandled = false;
};
