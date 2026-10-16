// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_MontageEventAbility.h"
#include "PP_Tertiary.generated.h"

class APP_FireballProjectile;

/**
 * 第三个技能（GA_Tertiary / 火球术）的 C++ 实现：
 * 事件到达后：Commit → 在 Avatar 前方生成火球投射物（APP_FireballProjectile）
 */
UCLASS()
class PERSONALPROJECT_API UPP_Tertiary : public UPP_MontageEventAbility
{
	GENERATED_BODY()

public:
	UPP_Tertiary() = default;

protected:
	virtual FGameplayTag GetEventTagToWait() const override;
	virtual void OnAbilityEventReceived(FGameplayEventData Payload) override;

	// 火球投射物类（继承 APP_FireballProjectile 的蓝图）
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<APP_FireballProjectile> ProjectileClass;

	// 投射物生成点：Avatar 前方偏移距离
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Crash|Abilities")
	float ProjectileSpawnForwardOffset = 100.0f;
};
