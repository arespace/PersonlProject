
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
	TObjectPtr<UPP_WaitGameplayEvent> WaitHitReactTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> SearchDelayTask;
	
	UPROPERTY()
	TObjectPtr<UAITask_MoveTo> MoveToLocationOrActorTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> AttackDelayTask;
	
	UPROPERTY()
	TObjectPtr<UAbilityTask_WaitDelay> PostAttackCooldownTask;

	/** 诊断：StartSearch 调用序号 */
	int32 SearchSerial{0};

	/** 诊断：已挂起但尚未到时的搜索 Delay 数量 */
	int32 SearchPending{0};

	/** 诊断：已发起但尚未回调的 MoveTo 数量 */
	int32 MoveToPending{0};

	/** 诊断：Attack 触发序号 */
	int32 AttackSerial{0};

	/** 诊断：上一次 Attack 的世界时间 */
	float LastAttackTime{-1.f};

	/** 下一次允许攻击的世界时间（冷却门控） */
	float NextAttackAllowedTime{-1.f};
	
	void StartSearch();
	
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
