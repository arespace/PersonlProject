// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "GameplayEffect.h"
#include "PP_Primary.generated.h"

class UAnimMontage;

/**
 * 
 */
UCLASS()
class PERSONALPROJECT_API UPP_Primary : public UPP_GameplayAbility
{
	GENERATED_BODY()
	
public:

	UPP_Primary();

	UFUNCTION(BlueprintCallable, Category= "Crash|Abilities")
	void SendHitReactEventToActors(const TArray<AActor*> ActorsHit);
	
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

protected:
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxRadius = 100.0f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxForwardOffset = 200.0f;
	
	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	float HitBoxElevationOffset = 20.0f;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> BlockHitReactEffectClass;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> LeftPunchMontage;

	UPROPERTY(EditDefaultsOnly,BlueprintReadOnly, Category = "Crash|Abilities")
	TObjectPtr<UAnimMontage> RightPunchMontage;

private:
	// FlipFlop 交替状态：每次激活切换左拳/右拳
	bool bUseLeftPunch = true;

	// 缓存激活参数，供蒙太奇结束回调里调用 EndAbility
	FGameplayAbilitySpecHandle CachedHandle;
	const FGameplayAbilityActorInfo* CachedActorInfo = nullptr;
	FGameplayAbilityActivationInfo CachedActivationInfo;

	UFUNCTION()
	void OnAttackMontageFinished();

	UFUNCTION()
	void OnGameplayEventReceived(FGameplayEventData Payload);
};
