// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_MontageEventAbility.h"
#include "PP_Primary.generated.h"

class UAnimMontage;

/**
 * 第一个技能（近战连击）的 C++ 实现：
 * 施加 BlockHitReact → FlipFlop 交替播放左/右拳蒙太奇 →
 * 监听 PPTags.Events.Player.Primary 事件 → HitBox 检测 → 通知受击 + 施加伤害 GE
 */
UCLASS()
class PERSONALPROJECT_API UPP_Primary : public UPP_MontageEventAbility
{
	GENERATED_BODY()

public:
	UPP_Primary();

	UFUNCTION(BlueprintCallable, Category= "Crash|Abilities")
	void SendHitReactEventToActors(const TArray<AActor*> ActorsHit);

protected:
	virtual UAnimMontage* GetMontageToPlay() override;
	virtual FGameplayTag GetEventTagToWait() const override;
	virtual void OnAbilityEventReceived(FGameplayEventData Payload) override;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxRadius = 100.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxForwardOffset = 200.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxElevationOffset = 20.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> LeftPunchMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> RightPunchMontage;

private:
	// FlipFlop 交替状态：每次激活切换左拳/右拳
	bool bUseLeftPunch = true;
};
