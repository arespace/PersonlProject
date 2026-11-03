// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "AbilitySystem/AbilityTasks/PP_WaitGameplayEvent.h"
#include "PP_SearchForTarget.generated.h"

namespace EPathFollowingResult
{
	enum Type : int;
}

class APP_BaseCharacter;
class APP_EnemyCharacter;
class AAIController;
class UPP_WaitGameplayEvent;
class UAbilityTask_WaitDelay;
class UAITask_MoveTo;

UCLASS()
class PERSONALPROJECT_API UPP_SearchForTarget : public UPP_GameplayAbility
{
	GENERATED_BODY()
	
public:
	UPP_SearchForTarget();
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData) override;
	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;
	
	TWeakObjectPtr<APP_EnemyCharacter> OwningEnemy;
	TWeakObjectPtr<AAIController> OwningAIController;

private:
	
	UPROPERTY()
	TObjectPtr<UPP_WaitGameplayEvent> WaitEndAttackTask;

	UPROPERTY()
	TObjectPtr<UPP_WaitGameplayEvent> WaitHitReactTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> SearchDelayTask;
	
	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveToLocationOrActorTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> AttackDelayTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> PostAttackCooldownTask;
	
	void StartSearch();
	
	UFUNCTION()
	void EndAttackEventReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	void HitReactEventReceived(FGameplayEventData Payload);
	
	UFUNCTION()
	void Search();
	
	UFUNCTION()
	void MoveToTargetAndAttack();
	
	UFUNCTION()
	void AttackTarget(TEnumAsByte<EPathFollowingResult::Type> Result, AAIController* AIController);

	UFUNCTION()
	void Attack();

	UFUNCTION()
	void OnPostAttackCooldown();
};
