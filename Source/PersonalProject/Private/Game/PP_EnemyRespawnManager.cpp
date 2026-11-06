// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_EnemyRespawnManager.h"

#include "Characters/PP_EnemyCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"

namespace
{
	/** 判定"同一条生成记录"时允许的生成点位置误差 */
	constexpr float SpawnMatchTolerance = 25.f;
}

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

	// 篝火重建敌人时，SpawnActor 会同步触发 BeginPlay → RegisterEnemy。
	// 此时由 ResetEnemiesInRadius 自己回填记录，跳过登记以免新增重复记录。
	if (bIsRespawning) return;

	UClass* EnemyClass = Enemy->GetClass();
	const FTransform SpawnTransform(Enemy->GetSpawnRotation(), Enemy->GetSpawnLocation());

	// 1. 已指向同一实例 → 刷新类与生成点
	for (FPP_EnemySpawnRecord& Record : EnemySpawnRecords)
	{
		if (Record.Enemy.Get() == Enemy)
		{
			Record.EnemyClass = EnemyClass;
			Record.SpawnTransform = SpawnTransform;
			return;
		}
	}

	// 2. 存在"实例已销毁 + 同类 + 同一生成点"的记录 → 接管该记录
	//    （例如蓝图重生逻辑自行在生成点重建了同款敌人）
	for (FPP_EnemySpawnRecord& Record : EnemySpawnRecords)
	{
		if (Record.Enemy.IsValid() || Record.EnemyClass != EnemyClass) continue;
		if (!Record.SpawnTransform.GetLocation().Equals(SpawnTransform.GetLocation(), SpawnMatchTolerance)) continue;

		Record.Enemy = Enemy;
		Record.SpawnTransform = SpawnTransform;
		return;
	}

	// 3. 全新记录
	FPP_EnemySpawnRecord& NewRecord = EnemySpawnRecords.AddDefaulted_GetRef();
	NewRecord.EnemyClass = EnemyClass;
	NewRecord.SpawnTransform = SpawnTransform;
	NewRecord.Enemy = Enemy;
}

void UPP_EnemyRespawnManager::UnregisterEnemy(APP_EnemyCharacter* Enemy)
{
	if (!Enemy) return;

	// 整条记录删除：该敌人永久移除，篝火休息不会再把它在生成点重建
	EnemySpawnRecords.RemoveAll([Enemy](const FPP_EnemySpawnRecord& Record)
	{
		return Record.Enemy.Get() == Enemy;
	});
}

void UPP_EnemyRespawnManager::RegisterAllEnemies()
{
	UWorld* World = GetWorld();
	if (!World) return;

	int32 FoundCount = 0;
	for (TActorIterator<APP_EnemyCharacter> It(World); It; ++It)
	{
		if (APP_EnemyCharacter* Enemy = *It)
		{
			if (IsValid(Enemy))
			{
				RegisterEnemy(Enemy);
				++FoundCount;
			}
		}
	}

	UE_LOG(LogPP, Log, TEXT("[EnemyRespawnManager] RegisterAllEnemies: 关卡内 %d 个敌人，注册表共 %d 条记录"),
		FoundCount, EnemySpawnRecords.Num());
}

int32 UPP_EnemyRespawnManager::ResetEnemiesInRadius(const FVector& Origin, float Radius)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 ResetCount = 0;
	TArray<int32> PendingRespawn;

	// 第一阶段：只读遍历，收集需要重建的记录索引。
	// 不能在循环内直接 SpawnActor：SpawnActor 会同步执行敌人 BeginPlay → RegisterEnemy，
	// 可能修改 EnemySpawnRecords，导致当前循环的引用失效。
	for (int32 Index = 0; Index < EnemySpawnRecords.Num(); ++Index)
	{
		const FPP_EnemySpawnRecord& Record = EnemySpawnRecords[Index];

		if (APP_EnemyCharacter* Enemy = Record.Enemy.Get())
		{
			if (FVector::Dist(Enemy->GetActorLocation(), Origin) <= Radius)
			{
				Enemy->ResetToSpawn();
				++ResetCount;
			}
			continue;
		}

		// 实例已销毁（死亡后被清除）：按记录的生成点判断是否在半径内
		if (!Record.EnemyClass) continue;
		if (FVector::Dist(Record.SpawnTransform.GetLocation(), Origin) > Radius) continue;

		PendingRespawn.Add(Index);
	}

	// 第二阶段：按记录重建已销毁的敌人
	bIsRespawning = true;
	for (int32 Index : PendingRespawn)
	{
		if (!EnemySpawnRecords.IsValidIndex(Index)) continue;

		const TSubclassOf<APP_EnemyCharacter> EnemyClass = EnemySpawnRecords[Index].EnemyClass;
		const FTransform SpawnTransform = EnemySpawnRecords[Index].SpawnTransform;
		if (!EnemyClass) continue;

		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		APP_EnemyCharacter* Respawned = World->SpawnActor<APP_EnemyCharacter>(EnemyClass, SpawnTransform, SpawnParams);
		if (Respawned)
		{
			EnemySpawnRecords[Index].Enemy = Respawned;
			++ResetCount;
			UE_LOG(LogPP, Log, TEXT("[EnemyRespawnManager] 重建敌人 %s @ (%.0f, %.0f, %.0f)"),
				*GetNameSafe(Respawned), SpawnTransform.GetLocation().X, SpawnTransform.GetLocation().Y,
				SpawnTransform.GetLocation().Z);
		}
		else
		{
			UE_LOG(LogPP, Warning, TEXT("[EnemyRespawnManager] 重建敌人失败，类=%s（记录保留，下次休息重试）"),
				*GetNameSafe(EnemyClass.Get()));
		}
	}
	bIsRespawning = false;

	UE_LOG(LogPP, Log, TEXT("[EnemyRespawnManager] ResetEnemiesInRadius: Origin=(%.0f,%.0f,%.0f) Radius=%.0f 重置 %d / 共 %d"),
		Origin.X, Origin.Y, Origin.Z, Radius, ResetCount, EnemySpawnRecords.Num());

	return ResetCount;
}

void UPP_EnemyRespawnManager::ClearAll()
{
	EnemySpawnRecords.Empty();
}
