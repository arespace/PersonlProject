// UI 网关接口 — 主玩法模块与 UI 模块的边界
// 模块化拆分后主玩法模块禁止 include UI 头文件，PlayerController 对 HUD 的全部操作
// 通过本接口转发：APP_HUD / APP_LobbyHUD（UI 模块）实现本接口，PC 运行时 Cast 调用。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Framework/PP_ScoreboardTypes.h"
#include "Framework/PP_LobbyTypes.h"
#include "Framework/PP_SessionManager.h"
#include "PP_UIGateway.generated.h"

class UUserWidget;

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

	// ===== 对话与商店 UI（NPC 参数用 UObject*，UI 侧经 IPP_DialogueSource 接口查询，消除对 Quest 模块具体类的依赖） =====
	// Widget 类由调用方（PlayerController）传入，避免 HUD（UI 模块）反向依赖 PlayerController
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestOpenDialogue(UObject* NPC, FName DialogueID, TSubclassOf<UUserWidget> WidgetClass);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestOpenShop(UObject* NPC, TSubclassOf<UUserWidget> ShopWidgetClass);

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
