// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "GameplayEffect.h"
#include "PP_Secondary.generated.h"

class UAnimMontage;
class UParticleSystem;
class UCameraShakeBase;

/**
 * 第二个技能（GA_Secondary）的 C++ 实现：
 * 施加 BlockHitReact → 播放 MeleeSecondary 蒙太奇 → 监听 PPTags.PPAbilities.Secondary 事件
 * → Commit → 粒子 + 镜头震动 + 命中检测 → 击退 → 伤害
 */
UCLASS()
class PERSONALPROJECT_API UPP_Secondary : public UPP_GameplayAbility
{
	GENERATED_BODY()

public:
	UPP_Secondary();

	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	// 命中检测（对应蓝图变量 "Hit Box Radius"，同时用作击退 OuterRadius）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxRadius = 1000.0f;

	// 击退参数（对应蓝图变量 "Inner Radius" / "Launch Force Magnitude"）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float InnerRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float LaunchForceMagnitude = 1000.0f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float RotationAngle = 65.0f;

	// 伤害数值（对应 SendDamageEventToPlayers 节点的 Damage 字面量）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float Damage = 10.0f;

	// GE 类（对应 GE_BlockHitReact / DamageGE）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> BlockHitReactEffectClass;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 表现资源（对应 AM_MeleeSecondary_Ability / P_ImpactBlast / BP_DeathCameraShake）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> AbilityMontage;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UParticleSystem> ImpactParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;

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
