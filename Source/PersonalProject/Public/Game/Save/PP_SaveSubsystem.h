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

	/** 采集指定玩家当前进度并写盘（服务端） */
	bool SavePlayer(APlayerController* PC, FString& OutError);

	/** 读盘并缓存为 PendingLoad；本方法不改任何运行时状态 */
	bool LoadFromDisk(FString& OutError);

	/**
	 * 把 PendingLoad 应用到指定玩家（服务端，一次性）。
	 * 当前批次应用：角色 ID、九项属性 base 值。
	 */
	bool ApplyPendingToPlayer(APlayerController* PC, FString& OutError);

	bool HasPendingLoad() const { return PendingSave != nullptr; }
	const UPP_SaveGame* GetPendingSave() const { return PendingSave; }
	void ClearPendingLoad() { PendingSave = nullptr; }

private:
	/** 待应用的读档结果（one-shot，应用成功即清除，避免重生/重连重复套用） */
	UPROPERTY(Transient)
	TObjectPtr<UPP_SaveGame> PendingSave;
};
