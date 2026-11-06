
#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayEffect.h"
#include "PP_GameplayAbility.generated.h"

/**
 * 
 */
UCLASS()
class PERSONALPROJECT_API UPP_GameplayAbility : public UGameplayAbility
{
	GENERATED_BODY()
	
public:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;

	/** 体力不足以支付 StaminaCost 时阻止激活（普攻等不消耗体力的技能保持 0） */
	virtual bool CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, FGameplayTagContainer* OptionalRelevantTags = nullptr) const override;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="PP|Debug")
	bool bDrawDebugs = false;

	/** 激活时消耗的体力值（0 = 不消耗体力） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina")
	float StaminaCost = 0.f;

	/**
	 * 体力消耗 GE：Instant 型，Modifier = PP_AttributeSet.Stamina + SetByCaller(PPTags.SetByCaller.StaminaCost)。
	 * StaminaCost > 0 时必须配置，否则只做拦截、不实际扣体力。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

private:
	/** 通过 StaminaCostEffect 扣除体力（SetByCaller 传 -StaminaCost） */
	void ApplyStaminaCost(const FGameplayAbilityActorInfo* ActorInfo) const;
};
