// UI 网关接口 — 主玩法模块与 UI 模块的边界
// 模块化拆分后主玩法模块禁止 include UI 头文件，PlayerController 对 HUD 的全部操作
// 通过本接口转发：APP_HUD / APP_LobbyHUD（UI 模块）实现本接口，PC 运行时 Cast 调用。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Framework/PP_ScoreboardTypes.h"
#include "Player/PP_LobbyPlayerState.h"
#include "Game/PP_SessionManager.h"
#include "PP_UIGateway.generated.h"

class APP_NPCActor;

UINTERFACE(BlueprintType)
class UPP_UIGateway : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_UIGateway — UI 网关接口
 * 实现方：APP_HUD（战斗 HUD）、APP_LobbyHUD（大厅 HUD），位于 PersonalProjectUI 模块
 * 调用方：APP_PlayerController（主玩法模块）
 */
class PERSONALPROJECT_API IPP_UIGateway
{
	GENERATED_BODY()

public:
	// ===== HUD 开关（Request 前缀避免与 HUD 现有 UFUNCTION 同名） =====
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestTogglePauseMenu();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestShowScoreboard();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestHideScoreboard();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestToggleMainHUD();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestToggleQuestLog();
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestToggleInventory();

	// ===== 通知与计分板 =====
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void PushNotification(const FString& Message);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void PushChatMessage(const FString& PlayerName, const FString& Message);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestUpdateScoreboard(const TArray<FScoreboardEntry>& Entries);

	// ===== 对话与商店 UI（NPC 参数用 Quest 模块类型，主玩法模块允许依赖 Quest） =====
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestOpenDialogue(APP_NPCActor* NPC, FName DialogueID);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestOpenShop(APP_NPCActor* NPC);

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
