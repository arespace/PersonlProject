
#pragma once

#include "CoreMinimal.h"
#include "Characters/PP_BaseCharacter.h"
#include "Interfaces/PP_InteractableInterface.h"
#include "PP_EnemyCharacter.generated.h"

class UAbilitySystemComponent;
class UAttributeSet;
class UPP_InventoryComponent;
class UPP_AIPerceptionComponent;
struct FPP_EnemyInfo;

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
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|AI")
	float AcceptanceRadius{500.f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|AI")
	float MinAttackDelay{.1f};
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|AI")
	float MaxAttackDelay{.5f};

	/** 攻击前的额外随机冷却窗口（秒）：MoveTo 完成后等 MinAttackDelay + 随机(AttackCooldownMin, AttackCooldownMax) */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|AI")
	float AttackCooldownMin{1.5f};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|AI")
	float AttackCooldownMax{2.5f};

	UFUNCTION(BlueprintImplementableEvent)
	float GetTimeLiveLenght();
	
	UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Replicated)
	bool bIsBeingLaunched{false};

	/** 死亡时掉落的金币数量（DT_Enemies.GoldDropAmount 有行时覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 GoldDropAmount = 10;

	/** 死亡时直接放入击杀者背包的物品 ID（DT_Enemies.DropItemID 有行时覆盖，空表示不发物品） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	FName DropItemID;

	/** 上述物品的发放数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Loot")
	int32 DropItemCount = 1;

	/** 死亡后是否复活（对应 GA_PP_Death 蓝图里的 ShouldRespawn 变量） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Death")
	bool bShouldRespawn = false;

	/** 敌人唯一标识符（任务系统用于 KillEnemy 目标匹配） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Quest")
	FName EnemyID;

	/** 访问 AIController 上挂的感知组件（可能 nullptr 若蓝图没挂） */
	UFUNCTION(BlueprintCallable, Category = "PP|AI")
	UPP_AIPerceptionComponent* GetAIPerceptionComponent() const;

	/** DT_Enemies 行的伤害覆盖值；<= 0 表示未配置，调用方应沿用原有伤害 */
	float GetTableDamage() const { return TableDamageOverride; }
	
	void StopMovementUntilLanded();

	/** 敌人初始生成位置（BeginPlay 缓存，供 ResetToSpawn 用） */
	UFUNCTION(BlueprintCallable, Category = "PP|AI")
	FVector GetSpawnLocation() const { return SpawnLocation; }

	/** 敌人初始生成朝向（BeginPlay 缓存） */
	UFUNCTION(BlueprintCallable, Category = "PP|AI")
	FRotator GetSpawnRotation() const { return SpawnRotation; }

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	/** 按 EnemyID 从 DT_Enemies 取行并覆盖本类数值；无行时保持蓝图配置不变 */
	virtual void ApplyEnemyDataConfig();

	/** 子类扩展的表格字段（Boss 阶段/名称等），基类不处理 */
	virtual void ApplyEnemyExtraData(const FPP_EnemyInfo& Row);

	/** 把 DT_Enemies 行的生命上限写入属性集；未配置（<=0）时保留 GE 初始化值 */
	void ApplyTableMaxHealth();

	/** DT_Enemies 行的生命上限，<=0 表示未配置 */
	float TableMaxHealthOverride{0.f};

	/** DT_Enemies 行的伤害，<=0 表示未配置 */
	float TableDamageOverride{0.f};

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
