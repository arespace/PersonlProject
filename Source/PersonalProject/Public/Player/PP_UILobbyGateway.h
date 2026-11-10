// UI 大厅网关接口 — 主玩法模块与 UI 模块在大厅上下文下的边界
// 大厅的玩家列表、房间设置、开局倒计时、房主迁移事件经本接口下发。
// 实现方：APP_LobbyHUD（PersonalProjectUI 模块）；调用方：APP_PlayerController（主玩法模块）。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Session/PP_LobbyTypes.h"
#include "PP_UILobbyGateway.generated.h"

UINTERFACE(BlueprintType)
class UPP_UILobbyGateway : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_UILobbyGateway — 大厅期 UI 网关
 * 实现方：APP_LobbyHUD（大厅 HUD）
 * 聊天/系统消息不在此接口，见 IPP_UIMessagingGateway。
 */
class PERSONALPROJECT_API IPP_UILobbyGateway
{
	GENERATED_BODY()

public:
	// ===== 大厅推送 =====
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void HandlePlayerListUpdated(const TArray<FLobbyPlayerInfo>& Players);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void HandleRoomSettingsUpdated(const FRoomSettings& Settings);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void HandleGameStarting(float Countdown);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void HandleHostMigrating(const FHostMigrationData& MigrationData);
};
