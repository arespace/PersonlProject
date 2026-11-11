// 存档系统 — 采集/应用调度
// 放 GameInstanceSubsystem：跨 ServerTravel 存活，大厅与战斗地图都能读档。
// 所有方法都要求服务端调用；客户端只经 PlayerController 的 Server RPC 请求。

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PP_SaveSubsystem.generated.h"

class APlayerController;
class UPP_SaveGame;

UCLASS()
class PERSONALPROJECT_API UPP_SaveSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/** 从 WorldContext 取子系统 */
	static UPP_SaveSubsystem* Get(const UObject* WorldContextObject);

	/** 采集指定玩家当前进度并写盘（服务端；同一帧内只写一次） */
	bool SavePlayer(APlayerController* PC, FString& OutError);

	/**
	 * 保存本机（房主/单人）玩家进度：自动从当前世界找本机 PlayerController。
	 * 用于篝火休息、返回大厅、退出游戏这类"手上没有 PC 引用"的时机。
	 */
	bool SaveLocalPlayer(FString& OutError);

	/** 读盘并缓存为 PendingLoad；本方法不改任何运行时状态 */
	bool LoadFromDisk(FString& OutError);

	/**
	 * 把 PendingLoad 应用到指定玩家（服务端，一次性）。
	 * 应用：角色 ID、背包/装备/金币、任务、九项属性 base 值、篝火检查点、世界状态（采集点/Boss/门/魂）。
	 * 世界状态与检查点只在存档地图与当前地图一致时应用。
	 */
	bool ApplyPendingToPlayer(APlayerController* PC, FString& OutError);

	/**
	 * 取走"读档后首次生成落点"（一次性）。
	 * 仅当应用存档时玩家还没有 Pawn（主菜单读档 → 进入地图）才会写入；
	 * 由 GameMode::RestartPlayer 消费，死亡复活流程不会用到。
	 */
	bool ConsumePendingSpawnTransform(FTransform& OutTransform);

	bool HasPendingLoad() const { return PendingSave != nullptr; }
	const UPP_SaveGame* GetPendingSave() const { return PendingSave; }
	void ClearPendingLoad() { PendingSave = nullptr; }

private:
	/** 待应用的读档结果（one-shot，应用成功即清除，避免重生/重连重复套用） */
	UPROPERTY(Transient)
	TObjectPtr<UPP_SaveGame> PendingSave;

	/** 读档后首次生成落点（仅当应用时没有 Pawn）；RestartPlayer 消费一次 */
	bool bHasPendingSpawnTransform = false;
	FTransform PendingSpawnTransform = FTransform::Identity;

	/** 上一次成功保存的帧号：同一帧内多个时机（篝火/返回大厅/退出）只写一次 */
	uint64 LastSaveFrame = 0;
};
