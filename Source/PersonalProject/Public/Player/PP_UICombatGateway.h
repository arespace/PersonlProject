// UI 战斗网关接口 — 主玩法模块与 UI 模块在战斗上下文下的边界
// 主玩法模块禁止 include UI 头文件，PlayerController 对战斗 HUD 的操作全部经本接口转发。
// 实现方：APP_HUD（PersonalProjectUI 模块）；调用方：APP_PlayerController（主玩法模块）。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Session/PP_ScoreboardTypes.h"
#include "PP_UICombatGateway.generated.h"

class UUserWidget;

UINTERFACE(BlueprintType)
class UPP_UICombatGateway : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_UICombatGateway — 战斗期 UI 网关
 * 实现方：APP_HUD（战斗 HUD）
 * 消息推送不在此接口，见 IPP_UIMessagingGateway；大厅事件见 IPP_UILobbyGateway。
 */
class PERSONALPROJECT_API IPP_UICombatGateway
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

	// ===== 计分板 =====
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestUpdateScoreboard(const TArray<FScoreboardEntry>& Entries);

	// ===== 对话与商店 UI（NPC 参数用 UObject*，UI 侧经 IPP_DialogueSource 接口查询，消除对 Quest 模块具体类的依赖） =====
	// Widget 类由调用方（PlayerController）传入，避免 HUD（UI 模块）反向依赖 PlayerController
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestOpenDialogue(UObject* NPC, FName DialogueID, TSubclassOf<UUserWidget> WidgetClass);
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void RequestOpenShop(UObject* NPC, TSubclassOf<UUserWidget> ShopWidgetClass);
};
