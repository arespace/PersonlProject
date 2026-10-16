// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_LobbyHUD.h"
#include "Lobby/PP_MainMenuWidget.h"
#include "Lobby/PP_LobbyWidget.h"
#include "Lobby/PP_HostSettingsWidget.h"
#include "Lobby/PP_CharacterSelectWidget.h"
#include "Lobby/PP_RoomSettingsWidget.h"
#include "Lobby/PP_SettingsWidget.h"
#include "Characters/PP_CharacterPreviewActor.h"
#include "Characters/PP_CharacterData.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_SessionManager.h"
#include "Blueprint/UserWidget.h"

APP_LobbyHUD::APP_LobbyHUD() {}

APP_LobbyHUD* APP_LobbyHUD::GetFromWidget(const UUserWidget* Widget)
{
	const APlayerController* PC = Widget ? Widget->GetOwningPlayer() : nullptr;
	return PC ? Cast<APP_LobbyHUD>(PC->GetHUD()) : nullptr;
}

void APP_LobbyHUD::BeginPlay()
{
	Super::BeginPlay();

	CreateWidgets();

	UWorld* World = GetWorld();
	if (World && World->GetNetMode() != NM_Standalone)
	{
		ShowLobby();
	}
	else
	{
		UPP_SessionManager* SM = UPP_SessionManager::Get(World);
		if (SM && SM->HasPendingMigration())
		{
			const FHostMigrationData& MigrationData = SM->GetMigrationData();
			if (MigrationData.bIsNewHost)
			{
				SM->ExecuteHostMigration_AsNewHost();
			}
			else
			{
				SM->ExecuteHostMigration_AsClient();
			}
		}
		else
		{
			ShowMainMenu();
		}
	}

	SetUIInputMode();
}

void APP_LobbyHUD::CreateWidgets()
{
	if (!PlayerOwner) return;

	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown) return;

	// 主菜单初始可见（房主迁移等不经过 ShowMainMenu 的分支需要它兜底显示）
	CreatePanelWidget(MainMenuWidget, MainMenuWidgetClass, ZOrder_MainMenu, /*bStartHidden=*/false);
	CreatePanelWidget(LobbyWidget, LobbyWidgetClass, ZOrder_Lobby);
	CreatePanelWidget(HostSettingsWidget, HostSettingsWidgetClass, ZOrder_Settings);
	CreatePanelWidget(CharacterSelectWidget, CharacterSelectWidgetClass, ZOrder_CharacterSelect);
	CreatePanelWidget(RoomSettingsWidget, RoomSettingsWidgetClass, ZOrder_Settings);
	CreatePanelWidget(SettingsWidget, SettingsWidgetClass, ZOrder_Settings);
}

void APP_LobbyHUD::HideAllMenus()
{
	if (MainMenuWidget) MainMenuWidget->SetVisibility(ESlateVisibility::Hidden);
	if (LobbyWidget) LobbyWidget->SetVisibility(ESlateVisibility::Hidden);
	if (HostSettingsWidget) HostSettingsWidget->SetVisibility(ESlateVisibility::Hidden);
	if (CharacterSelectWidget) CharacterSelectWidget->SetVisibility(ESlateVisibility::Hidden);
	if (RoomSettingsWidget) RoomSettingsWidget->SetVisibility(ESlateVisibility::Hidden);
	if (SettingsWidget) SettingsWidget->SetVisibility(ESlateVisibility::Hidden);
}

void APP_LobbyHUD::ShowMainMenu()
{
	ShowPanel(MainMenuWidget);

	// 每次显示主菜单自动搜一次房间，避免房间列表初始为空
	if (MainMenuWidget)
	{
		MainMenuWidget->RefreshRoomList();
	}
}

bool APP_LobbyHUD::IsInRoom() const
{
	const UWorld* World = GetWorld();
	return World && World->GetNetMode() != NM_Standalone;
}

void APP_LobbyHUD::ShowLobby()
{
	ShowPanel(LobbyWidget);
	ShowPreviewActor();

	// 正常进入大厅时清掉上一次的迁移提示
	if (LobbyWidget)
	{
		LobbyWidget->SetMigrationBannerVisible(false);
	}
}

void APP_LobbyHUD::ShowHostSettings()
{
	ShowPanel(HostSettingsWidget);
}

void APP_LobbyHUD::HideHostSettings()
{
	ShowMainMenu();
}

void APP_LobbyHUD::ShowCharacterSelect()
{
	ShowPanel(CharacterSelectWidget);
	ShowPreviewActor();
}

void APP_LobbyHUD::HideCharacterSelect()
{
	if (PreviewActor)
	{
		PreviewActor->SetActorHiddenInGame(true);
	}
	ShowLobby();
}

void APP_LobbyHUD::ShowRoomSettings()
{
	ShowPanel(RoomSettingsWidget);
}

void APP_LobbyHUD::HideRoomSettings()
{
	ShowLobby();
}

void APP_LobbyHUD::ShowSettings()
{
	ShowPanel(SettingsWidget);
}

void APP_LobbyHUD::HideSettings()
{
	ShowMainMenu();
}

void APP_LobbyHUD::UpdatePreviewCharacter(FName CharacterId)
{
	if (!PreviewActor) return;

	// 统一走 DataManager 全局角色表；表不可用时与原先表引用为空一致：直接返回且不更新本地缓存
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	if (!DataManager || !DataManager->HasCharacterTable()) return;

	LocalSelectedCharacterId = CharacterId;

	if (const FCharacterDataRow* Row = DataManager->GetCharacterData(CharacterId))
	{
		PreviewActor->SetPreviewCharacter(*Row);
	}
}

void APP_LobbyHUD::OnPlayerListUpdated(const TArray<FLobbyPlayerInfo>& Players)
{
	if (LobbyWidget) LobbyWidget->UpdatePlayerList(Players);
}

void APP_LobbyHUD::OnRoomSettingsUpdated(const FRoomSettings& Settings)
{
	if (RoomSettingsWidget) { RoomSettingsWidget->ApplySettings(Settings); }
	// 大厅也要拿到 MaxPlayers / MapName / GameModeName 用于顶栏显示与空位补齐
	if (LobbyWidget) { LobbyWidget->ApplyRoomSettings(Settings); }
}

void APP_LobbyHUD::NotifyHostMigrating()
{
	if (LobbyWidget) { LobbyWidget->SetMigrationBannerVisible(true); }
}

void APP_LobbyHUD::OnGameStarting(float Countdown)
{
	if (LobbyWidget) { LobbyWidget->OnGameStarting(Countdown); }
}

void APP_LobbyHUD::AddChatMessage(const FString& PlayerName, const FString& Message)
{
	if (LobbyWidget) { LobbyWidget->AddChatMessage(PlayerName, Message); }
}

void APP_LobbyHUD::AddSystemMessage(const FString& Message)
{
	if (LobbyWidget) { LobbyWidget->AddSystemMessage(Message); }
}

void APP_LobbyHUD::SetUIInputMode()
{
	if (PlayerOwner)
	{
		PlayerOwner->SetShowMouseCursor(true);
		PlayerOwner->SetInputMode(FInputModeUIOnly());
	}
}

void APP_LobbyHUD::ShowPreviewActor()
{
	if (!PreviewActor && PreviewActorClass && GetWorld())
	{
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		PreviewActor = GetWorld()->SpawnActor<APP_CharacterPreviewActor>(
			PreviewActorClass, PreviewSpawnLocation, PreviewSpawnRotation, SpawnParams);
	}

	if (PreviewActor)
	{
		PreviewActor->SetActorHiddenInGame(false);

		if (LocalSelectedCharacterId.IsNone())
		{
			LocalSelectedCharacterId = FName("Warrior");
		}
		UpdatePreviewCharacter(LocalSelectedCharacterId);
	}
}

UTextureRenderTarget2D* APP_LobbyHUD::GetPreviewRenderTarget() const
{
	return PreviewActor ? PreviewActor->GetPreviewRenderTarget() : nullptr;
}

void APP_LobbyHUD::RotatePreviewCharacter(float YawDelta, float PitchDelta)
{
	if (PreviewActor)
	{
		PreviewActor->RotatePreview(YawDelta, PitchDelta);
	}
}
