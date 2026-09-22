// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "PP_BaseCharacter.h"
#include "PP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UPP_InventoryComponent;

UCLASS()
class PERSONALPROJECT_API APP_EnemyCharacter : public APP_BaseCharacter
{
	GENERATED_BODY()

public:
	APP_EnemyCharacter();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float AcceptanceRadius{500.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MinAttackDelay{.1f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MaxAttackDelay{.5f};
	UFUNCTION(BlueprintImplementableEvent)
	float GetTimeLiveLenght();
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Replicated)
	bool bIsBeingLaunched{false};

	/** 死亡时掉落的金币数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 GoldDropAmount = 10;

	/** 死亡后是否复活（对应 GA_PP_Death 蓝图里的 ShouldRespawn 变量） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|Death")
	bool bShouldRespawn = false;

	/** 敌人唯一标识符（任务系统用于 KillEnemy 目标匹配） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName EnemyID;
	
	void StopMovementUntilLanded();
protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;
	
private:
	
	UFUNCTION()
	void EnableMovementOnLanded(const FHitResult& Hit);
	
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

};
