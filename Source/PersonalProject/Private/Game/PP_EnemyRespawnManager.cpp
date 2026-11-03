// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_EnemyRespawnManager.h"

#include "Characters/PP_EnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"
#include "GameFramework/GameModeBase.h"

UPP_EnemyRespawnManager::UPP_EnemyRespawnManager()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPP_EnemyRespawnManager::BeginPlay()
{
	Super::BeginPlay();

	// 只在服务端跑（GameMode 是 Authority only）
	if (!GetOwner() || !GetOwner()->HasAuthority()) return;

	RegisterAllEnemies();
}

void UPP_EnemyRespawnManager::RegisterEnemy(APP_EnemyCharacter* Enemy)
{
	if (!IsValid(Enemy)) return;
	if (!EnemiesInLevel.Contains(Enemy))
	{
		EnemiesInLevel.Add(Enemy);
	}
}

void UPP_EnemyRespawnManager::UnregisterEnemy(APP_EnemyCharacter* Enemy)
{
	if (!Enemy) return;
	EnemiesInLevel.Remove(Enemy);
}

void UPP_EnemyRespawnManager::RegisterAllEnemies()
{
	EnemiesInLevel.Empty();
	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<APP_EnemyCharacter> It(World); It; ++It)
	{
		APP_EnemyCharacter* Enemy = *It;
		if (IsValid(Enemy))
		{
			EnemiesInLevel.Add(Enemy);
		}
	}

	UE_LOG(LogPP, Log, TEXT("[EnemyRespawnManager] RegisterAllEnemies: 注册 %d 个敌人"), EnemiesInLevel.Num());
}

int32 UPP_EnemyRespawnManager::ResetEnemiesInRadius(const FVector& Origin, float Radius)
{
	int32 ResetCount = 0;

	for (APP_EnemyCharacter* Enemy : EnemiesInLevel)
	{
		if (!IsValid(Enemy))
		{
			continue;
		}

		if (FVector::Dist(Enemy->GetActorLocation(), Origin) <= Radius)
		{
			Enemy->ResetToSpawn();
			++ResetCount;
		}
	}

	UE_LOG(LogPP, Log, TEXT("[EnemyRespawnManager] ResetEnemiesInRadius: Origin=(%.0f,%.0f,%.0f) Radius=%.0f 重置 %d / 共 %d"),
		Origin.X, Origin.Y, Origin.Z, Radius, ResetCount, EnemiesInLevel.Num());

	return ResetCount;
}

void UPP_EnemyRespawnManager::ClearAll()
{
	EnemiesInLevel.Empty();
}
