
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PP_EnemyRespawnManager.generated.h"

class APP_EnemyCharacter;

/**
 * 一条敌人生成记录。
 * 记录独立于 Actor 生命周期：敌人死亡后被销毁，篝火休息仍可据此在生成点重建。
 */
USTRUCT()
struct FPP_EnemySpawnRecord
{
	GENERATED_BODY()

	/** 敌人蓝图类（重建时用） */
	UPROPERTY()
	TSubclassOf<APP_EnemyCharacter> EnemyClass;

	/** 生成点变换 */
	UPROPERTY()
	FTransform SpawnTransform;

	/** 当前实例（死亡销毁后自动变空） */
	UPROPERTY()
	TWeakObjectPtr<APP_EnemyCharacter> Enemy;
};

/**
 * 关卡内敌人统一注册表 + 篝火休息重置派发器。
 * 由 C++ 在 APP_CrashGameMode 里默认创建，无需蓝图手动添加。
 *
 * 使用流程：
 *   1. 敌人 BeginPlay 自动 RegisterEnemy（记录类 + 生成点）
 *   2. BonfireActor::Rest() 调 ResetEnemiesInRadius(Location, Radius)
 *   3. 半径内活着的敌人 ResetToSpawn；已死亡销毁的按记录重建
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_EnemyRespawnManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_EnemyRespawnManager();

	/** 注册一个敌人（敌人 BeginPlay 自动调用） */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void RegisterEnemy(APP_EnemyCharacter* Enemy);

	/** 取消注册（敌人被永久移除，不再参与篝火重置） */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void UnregisterEnemy(APP_EnemyCharacter* Enemy);

	/** 扫描关卡内所有敌人并注册 */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void RegisterAllEnemies();

	/** 半径内敌人全部回生成点满血；已死亡销毁的按记录重建 */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	int32 ResetEnemiesInRadius(const FVector& Origin, float Radius);

	/** 清空注册表 */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void ClearAll();

	/** 已记录敌人数（含死亡待重建） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|EnemyRespawn")
	int32 GetEnemyCount() const { return EnemySpawnRecords.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	/** 关卡内所有敌人的生成记录 */
	UPROPERTY()
	TArray<FPP_EnemySpawnRecord> EnemySpawnRecords;

	/** 重建中：SpawnActor 触发的 BeginPlay 不要重复登记 */
	bool bIsRespawning{false};
};
