// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PP_EnemyRespawnManager.generated.h"

class APP_EnemyCharacter;

/**
 * 关卡内敌人统一注册表 + 篝火休息重置派发器。
 * 挂到 GameMode（蓝图子类里 Add Component）。
 *
 * 使用流程：
 *   1. GameMode 的蓝图子类 Add UPP_EnemyRespawnManager（C++ 也可以手动 RegisterAllEnemies）
 *   2. RespawnManager 在 BeginPlay 时 GetAllActorsOfClass<APP_EnemyCharacter> 注册
 *   3. BonfireActor::Rest() 通过 GetAuthGameMode()->FindComponentByClass<UPP_EnemyRespawnManager>()
 *      拿到它，调 ResetEnemiesInRadius(Location, Radius)
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_EnemyRespawnManager : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_EnemyRespawnManager();

	/** 注册一个敌人（关卡蓝图 Spawn 时手动调，或自动批量） */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void RegisterEnemy(APP_EnemyCharacter* Enemy);

	/** 取消注册（敌人被永久销毁时） */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void UnregisterEnemy(APP_EnemyCharacter* Enemy);

	/** BeginPlay 时自动扫关卡内所有敌人并注册 */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void RegisterAllEnemies();

	/** 半径内所有敌人执行 ResetToSpawn */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	int32 ResetEnemiesInRadius(const FVector& Origin, float Radius);

	/** 清空注册表 */
	UFUNCTION(BlueprintCallable, Category = "PP|EnemyRespawn")
	void ClearAll();

	/** 当前注册数量 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|EnemyRespawn")
	int32 GetEnemyCount() const { return EnemiesInLevel.Num(); }

protected:
	virtual void BeginPlay() override;

private:
	/** 关卡内所有活着的敌人 */
	UPROPERTY()
	TArray<TObjectPtr<APP_EnemyCharacter>> EnemiesInLevel;
};
