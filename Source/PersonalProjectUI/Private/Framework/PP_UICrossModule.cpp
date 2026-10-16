// IPP_UIGateway 接口实现（APP_HUD / APP_LobbyHUD）
//
// 模块化拆分：主玩法模块禁止 include UI 头文件，PlayerController 对 HUD 的全部操作
// 通过 IPP_UIGateway 接口（定义在主玩法模块）转发到本文件中的实现。
//   - APP_HUD：战斗 HUD 的全部网关方法（暂停菜单 / 计分板 / 通知 / 对话 / 商店 UI）
//   - APP_LobbyHUD：大厅推送方法（玩家列表 / 房间设置 / 开局倒计时 / 房主迁移 / 聊天）
// PC 的 Client RPC 实现体位于主玩法模块，仅做 Cast<IPP_UIGateway>(GetHUD()) 转发。

#include "HUD/PP_HUD.h"
#include "Framework/PP_UIUserWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Widgets/PP_ScoreboardWidget.h"
#include "Dialogue/PP_DialogueWidget.h"
#include "Shop/PP_ShopWidget.h"
#include "Dialogue/PP_NPCActor.h"
#include "Data/PP_DialogueData.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Shop/PP_ShopComponent.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "Engine/DataTable.h"
#include "Framework/PP_Log.h"

// ============================================================
// APP_HUD — 战斗 HUD 网关实现
// ============================================================

void APP_HUD::RequestTogglePauseMenu_Implementation()
{
	if (IsPauseMenuVisible())
	{
		UE_LOG(LogPP, Log, TEXT("[HUD Gateway] TogglePauseMenu - Hiding pause menu"));
		HidePauseMenu();
	}
	else
	{
		UE_LOG(LogPP, Log, TEXT("[HUD Gateway] TogglePauseMenu - Showing pause menu"));
		ShowPauseMenu();
	}
}

void APP_HUD::RequestShowScoreboard_Implementation()
{
	ShowScoreboard();
}

void APP_HUD::RequestHideScoreboard_Implementation()
{
	HideScoreboard();
}

void APP_HUD::RequestToggleMainHUD_Implementation()
{
	ToggleMainHUD();
}

void APP_HUD::RequestToggleQuestLog_Implementation()
{
	ToggleQuestLog();
}

void APP_HUD::RequestToggleInventory_Implementation()
{
	ToggleInventory();
}

void APP_HUD::PushNotification_Implementation(const FString& Message)
{
	AddNotification(Message);
}

void APP_HUD::PushChatMessage_Implementation(const FString& PlayerName, const FString& Message)
{
	AddNotification(FString::Printf(TEXT("[%s]: %s"), *PlayerName, *Message));
}

void APP_HUD::RequestUpdateScoreboard_Implementation(const TArray<FScoreboardEntry>& Entries)
{
	if (UPP_ScoreboardWidget* Scoreboard = GetScoreboardWidget())
	{
		Scoreboard->UpdateScoreboard(Entries);
	}
}

void APP_HUD::HandlePlayerListUpdated_Implementation(const TArray<FLobbyPlayerInfo>& Players)
{
	// 战斗 HUD 不处理大厅玩家列表
}

void APP_HUD::HandleRoomSettingsUpdated_Implementation(const FRoomSettings& Settings)
{
	// 战斗 HUD 不处理房间设置
}

void APP_HUD::HandleGameStarting_Implementation(float Countdown)
{
	// 战斗 HUD 不处理开局倒计时
}

void APP_HUD::HandleHostMigrating_Implementation(const FHostMigrationData& MigrationData)
{
	// 战斗 HUD 不处理房主迁移提示
}

void APP_HUD::RequestOpenDialogue_Implementation(APP_NPCActor* NPC, FName DialogueID, TSubclassOf<UUserWidget> WidgetClass)
{
	if (!IsValid(NPC))
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenDialogue - NPC is null"));
		return;
	}

	// 从 NPC 的对话表中查找行数据
	UDataTable* DT = NPC->GetDialogueTable();
	if (!IsValid(DT))
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenDialogue - NPC DialogueTable is null"));
		return;
	}

	FPP_DialogueData* Row = DT->FindRow<FPP_DialogueData>(DialogueID, TEXT("HUD Gateway OpenDialogue"));
	if (!Row)
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenDialogue - DialogueID '%s' not found in table"), *DialogueID.ToString());
		return;
	}

	// 确保 DialogueID 字段被正确填充（DataTable 行键可能已设置，但成员变量可能为空）
	FPP_DialogueData DataCopy = *Row;
	if (DataCopy.DialogueID.IsNone())
	{
		DataCopy.DialogueID = DialogueID;
	}

	// Widget 蓝图类由调用方（PlayerController）随网关调用传入，避免 HUD（UI 模块）反向依赖 PlayerController
	if (!WidgetClass)
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenDialogue - DialogueWidgetClass not configured"));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	UPP_DialogueWidget* DialogueWidget = Cast<UPP_DialogueWidget>(CreateWidget(PC, WidgetClass));
	if (!DialogueWidget)
	{
		UE_LOG(LogPP, Error, TEXT("[HUD Gateway] OpenDialogue - Failed to create DialogueWidget"));
		return;
	}

	DialogueWidget->AddToViewport(90);
	DialogueWidget->SetupDialogue(DataCopy, NPC, PC);

	UE_LOG(LogPP, Log, TEXT("[HUD Gateway] OpenDialogue - Opened dialogue '%s' with NPC '%s'"),
		*DialogueID.ToString(), *NPC->GetName());
}

void APP_HUD::RequestOpenShop_Implementation(APP_NPCActor* NPC, TSubclassOf<UUserWidget> ShopWidgetClass)
{
	if (!IsValid(NPC))
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenShop - NPC is null"));
		return;
	}

	UPP_ShopComponent* Shop = NPC->GetShopComponent();
	if (!IsValid(Shop))
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenShop - NPC ShopComponent is null"));
		return;
	}

	// Widget 蓝图类由调用方（PlayerController）传入
	if (!ShopWidgetClass)
	{
		UE_LOG(LogPP, Warning, TEXT("[HUD Gateway] OpenShop - ShopWidgetClass not configured"));
		return;
	}

	APlayerController* PC = GetOwningPlayerController();
	UPP_ShopWidget* ShopWidget = Cast<UPP_ShopWidget>(CreateWidget(PC, ShopWidgetClass));
	if (!ShopWidget)
	{
		UE_LOG(LogPP, Error, TEXT("[HUD Gateway] OpenShop - Failed to create ShopWidget"));
		return;
	}

	// 获取玩家的 InventoryComponent（挂在 PlayerState 上；用基类 APlayerState，避免依赖主玩法 PlayerState 类型）
	UPP_InventoryComponent* Inventory = nullptr;
	if (PC)
	{
		if (APlayerState* PS = PC->GetPlayerState<APlayerState>())
		{
			Inventory = PS->FindComponentByClass<UPP_InventoryComponent>();
		}
	}

	ShopWidget->AddToViewport(static_cast<int32>(EPP_UILayer::Overlay));
	ShopWidget->InitializeShop(Shop, Inventory);

	UE_LOG(LogPP, Log, TEXT("[HUD Gateway] OpenShop - Opened shop for NPC '%s'"), *NPC->GetName());
}

// ============================================================
// APP_LobbyHUD — 大厅 HUD 网关实现
// ============================================================

void APP_LobbyHUD::PushChatMessage_Implementation(const FString& PlayerName, const FString& Message)
{
	AddChatMessage(PlayerName, Message);
}

void APP_LobbyHUD::HandlePlayerListUpdated_Implementation(const TArray<FLobbyPlayerInfo>& Players)
{
	OnPlayerListUpdated(Players);
}

void APP_LobbyHUD::HandleRoomSettingsUpdated_Implementation(const FRoomSettings& Settings)
{
	OnRoomSettingsUpdated(Settings);
}

void APP_LobbyHUD::HandleGameStarting_Implementation(float Countdown)
{
	OnGameStarting(Countdown);
}

void APP_LobbyHUD::HandleHostMigrating_Implementation(const FHostMigrationData& MigrationData)
{
	NotifyHostMigrating();
}
