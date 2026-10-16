// 大厅操作请求接口 — UI 模块 → 主玩法 PlayerController 的边界
// 解耦：大厅/角色选择/房间设置等 Widget（UI 模块）不再 include PlayerController 头文件，
// 而是 Cast 到本接口后调用 UHT 静态分发器 Execute_Xxx，由 APP_PlayerController 实现并转发到 Server RPC。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Framework/PP_LobbyTypes.h"
#include "PP_LobbyActions.generated.h"

UINTERFACE(BlueprintType)
class UPP_LobbyActions : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_LobbyActions — 大厅阶段 UI 操作请求接口
 * 实现方：APP_PlayerController（主玩法模块）
 * 调用方：UPP_LobbyWidget / UPP_CharacterSelectWidget / UPP_RoomSettingsWidget（UI 模块）
 */
class PERSONALPROJECTCORE_API IPP_LobbyActions
{
	GENERATED_BODY()

public:
	/** 设置准备状态 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	void RequestSetReady(bool bReady);

	/** 请求开始游戏（仅房主） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	void RequestStartGame();

	/** 房主请求离开并触发迁移 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	void RequestHostMigration();

	/** 发送聊天消息 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	void RequestSendChatMessage(const FString& Message);

	/** 更新房间设置（仅房主） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	void RequestUpdateRoomSettings(const FRoomSettings& Settings);

	/** 选择角色 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	void RequestSelectCharacter(FName CharacterId);
};
