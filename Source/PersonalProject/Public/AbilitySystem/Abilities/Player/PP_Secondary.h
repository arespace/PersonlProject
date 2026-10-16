// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_MontageEventAbility.h"
#include "PP_Secondary.generated.h"

class UParticleSystem;
class UCameraShakeBase;

/**
 * 第二个技能（GA_Secondary）的 C++ 实现：
 * 事件到达后：Commit → 粒子 + 镜头震动 + 命中检测 → 击退 → 伤害
 */
UCLASS()
class PERSONALPROJECT_API UPP_Secondary : public UPP_MontageEventAbility
{
	GENERATED_BODY()

public:
	UPP_Secondary() = default;

protected:
	virtual FGameplayTag GetEventTagToWait() const override;
	virtual void OnAbilityEventReceived(FGameplayEventData Payload) override;

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

	// 伤害 GE（对应 DamageGE）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	// 表现资源（对应 P_ImpactBlast / BP_DeathCameraShake）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UParticleSystem> ImpactParticle;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UCameraShakeBase> CameraShakeClass;
};
