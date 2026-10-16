// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "Framework/PP_LobbyTypes.h"
#include "PP_LobbyGameMode.generated.h"

/**
 * APP_LobbyGameMode - 大厅阶段GameMode
 * 管理玩家加入/离开、Ready检查、房间设置同步、开始游戏
 * 使用SpectatorPawn作为默认Pawn，大厅不需要战斗角色
 */
UCLASS()
class PERSONALPROJECT_API APP_LobbyGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	APP_LobbyGameMode();

	// ===== 生命周期 =====

	virtual void InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage) override;
	virtual void PostLogin(APlayerController* NewPlayer) override;
	virtual void Logout(AController* Exiting) override;
	virtual FString InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal) override;

	// ===== 游戏流程 =====

	/** 检查所有玩家是否Ready */
	bool CheckAllPlayersReady() const;

	/** 开始游戏（主机调用），ServerTravel到战斗地图 */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void StartGame();

	/** 处理房主离开时的迁移（服务端调用） */
	void HandleHostMigration(AController* LeavingHost);

	/** 更新房间设置（仅主机可调用） */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void UpdateRoomSettings(const FRoomSettings& NewSettings);

	/** 获取当前房间设置 */
	FRoomSettings GetRoomSettings() const { return RoomSettings; }

	/** 获取所有大厅玩家信息 */
	TArray<FLobbyPlayerInfo> GetLobbyPlayerList() const;

	/** 广播玩家列表更新给所有客户端 */
	void BroadcastPlayerListUpdate();

protected:

	/** 房间设置 */
	UPROPERTY(ReplicatedUsing = OnRep_RoomSettings)
	FRoomSettings RoomSettings;

private:

	UFUNCTION()
	void OnRep_RoomSettings();

	/** 延迟断开房主连接（等待 RPC 送达） */
	UFUNCTION()
	void ExecuteHostDisconnect();

	/** 房主断开延迟定时器 */
	FTimerHandle HostDisconnectTimer;

	/** 分配玩家序号 */
	void AssignPlayerIndices();
};