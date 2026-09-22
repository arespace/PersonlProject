// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "GameplayEffect.h"
#include "PP_Tertiary.generated.h"

class UAnimMontage;
class APP_FireballProjectile;

/**
 * 第三个技能（GA_Tertiary / 火球术）的 C++ 实现：
 * 播放施法蒙太奇 → 监听 PPTags.PPAbilities.Tertiary 事件（AnimNotify）
 * → Commit → 在 Avatar 前方生成火球投射物（APP_FireballProjectile）
 */
UCLASS()
class PERSONALPROJECT_API UPP_Tertiary : public UPP_GameplayAbility
{
	GENERATED_BODY()

public:
	UPP_Tertiary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

	// 施法期间阻止受击反应（对应 GE_BlockHitReact，与 Secondary 一致）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> BlockHitReactEffectClass;

	// 施法蒙太奇（对应 CreatePlayMontageAndWaitProxy 的 MontageToPlay）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> AbilityMontage;

	// 火球投射物类（继承 APP_FireballProjectile 的蓝图）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<APP_FireballProjectile> ProjectileClass;

	// 投射物生成点：Avatar 前方偏移距离
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float ProjectileSpawnForwardOffset = 100.0f;

private:
	// 缓存激活参数，供蒙太奇结束回调里调用 EndAbility / CommitAbility
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	UFUNCTION()
	void OnAbilityMontageFinished();

	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);
};
