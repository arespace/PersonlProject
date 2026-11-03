// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Characters/PP_BaseCharacter.h"
#include "Interfaces/PP_InteractableInterface.h"
#include "PP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UPP_InventoryComponent;
class UPP_AIPerceptionComponent;

UCLASS()
class PERSONALPROJECT_API APP_EnemyCharacter : public APP_BaseCharacter, public IPP_Resettable
{
	GENERATED_BODY()

public:
	APP_EnemyCharacter();
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;

	//~ Begin IPP_Resettable
	virtual void ResetToSpawn_Implementation() override;
	//~ End IPP_Resettable
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float AcceptanceRadius{500.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MinAttackDelay{.1f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float MaxAttackDelay{.5f};

	/** 攻击前的额外随机冷却窗口（秒）：MoveTo 完成后等 MinAttackDelay + 随机(AttackCooldownMin, AttackCooldownMax) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float AttackCooldownMin{1.5f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|AI")
	float AttackCooldownMax{2.5f};

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

	/** 访问 AIController 上挂的感知组件（可能 nullptr 若蓝图没挂） */
	UFUNCTION(BlueprintCallable, Category = "Crash|AI")
	UPP_AIPerceptionComponent* GetAIPerceptionComponent() const;
	
	void StopMovementUntilLanded();

	/** 敌人初始生成位置（BeginPlay 缓存，供 ResetToSpawn 用） */
	UFUNCTION(BlueprintCallable, Category = "Crash|AI")
	FVector GetSpawnLocation() const { return SpawnLocation; }

	/** 敌人初始生成朝向（BeginPlay 缓存） */
	UFUNCTION(BlueprintCallable, Category = "Crash|AI")
	FRotator GetSpawnRotation() const { return SpawnRotation; }

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

	/** 缓存 BeginPlay 时的生成位置 */
	FVector SpawnLocation{FVector::ZeroVector};

	/** 缓存 BeginPlay 时的生成朝向 */
	FRotator SpawnRotation{FRotator::ZeroRotator};
};
