// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "PP_AbilitySystemComponent.generated.h"


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PERSONALPROJECTCORE_API UPP_AbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	virtual void OnGiveAbility(FGameplayAbilitySpec& AbilitySpec) override;
	virtual void OnRep_ActivateAbilities() override;
	
	UFUNCTION(BlueprintCallable,Category = "Crash|Abilities")
	void SetAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level);
	
	UFUNCTION(BlueprintCallable,Category = "Crash|Abilities")
	void AddToAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1);

	// ===== UI冷却查询接口 =====

	/** 获取指定能力Tag的冷却剩余时间 */
	UFUNCTION(BlueprintPure, Category = "Crash|Abilities|Cooldown")
	float GetCooldownRemainingForTag(const FGameplayTag& CooldownTag) const;

	/** 获取指定能力Tag的冷却总时间 */
	UFUNCTION(BlueprintPure, Category = "Crash|Abilities|Cooldown")
	float GetCooldownDurationForTag(const FGameplayTag& CooldownTag) const;

private:
	void HandlAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec);

};
