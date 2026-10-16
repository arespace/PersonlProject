// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_HostSettingsWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Framework/PP_SessionManager.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"
#include "Framework/PP_Log.h"

void UPP_HostSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (MapSelector) { MapSelector->AddOption(TEXT("PPMap")); MapSelector->SetSelectedOption(TEXT("PPMap")); }
	if (MaxPlayersSpinBox) { MaxPlayersSpinBox->SetMinValue(2.f); MaxPlayersSpinBox->SetMaxValue(16.f); MaxPlayersSpinBox->SetValue(4.f); }
	if (GameModeSelector) { GameModeSelector->AddOption(TEXT("Crash")); GameModeSelector->SetSelectedOption(TEXT("Crash")); }
	if (ConfirmButton) ConfirmButton->OnClicked.AddDynamic(this, &UPP_HostSettingsWidget::OnConfirmClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UPP_HostSettingsWidget::OnBackClicked);
}

void UPP_HostSettingsWidget::NativeDestruct()
{
	if (SessionManager)
	{
		SessionManager->OnCreateSessionComplete.RemoveDynamic(this, &UPP_HostSettingsWidget::OnCreateSessionCompleted);
	}

	Super::NativeDestruct();
}

APP_LobbyHUD* UPP_HostSettingsWidget::GetLobbyHUD() const
{
	return APP_LobbyHUD::GetFromWidget(this);
}

void UPP_HostSettingsWidget::OnConfirmClicked()
{
	// 获取玩家名
	APP_LobbyHUD* LobbyHUD = GetLobbyHUD();
	FString PlayerName = LobbyHUD ? LobbyHUD->GetPendingPlayerName() : TEXT("Player");
	if (PlayerName.IsEmpty()) PlayerName = TEXT("Player");

	// 获取最大玩家数
	int32 MaxPlayers = MaxPlayersSpinBox ? FMath::RoundToInt(MaxPlayersSpinBox->GetValue()) : 4;

	// 构造待执行的 Travel URL（Session创建成功后执行）
	PendingTravelURL = FString::Printf(TEXT("Open LobbyMap?Listen?PlayerName=%s"), *PlayerName);

	// 从 GameInstance Subsystem 获取 SessionManager（如果尚未获取）
	if (!SessionManager)
	{
		SessionManager = UPP_SessionManager::Get(GetWorld());

		if (SessionManager)
		{
			if (!SessionManager->OnCreateSessionComplete.IsAlreadyBound(this, &UPP_HostSettingsWidget::OnCreateSessionCompleted))
			{
				SessionManager->OnCreateSessionComplete.AddDynamic(this, &UPP_HostSettingsWidget::OnCreateSessionCompleted);
			}
		}
		else
		{
			UE_LOG(LogPP, Error, TEXT("[HostSettings] Failed to get SessionManager Subsystem!"));
			return;
		}
	}

	// 通过 SessionManager 注册 LAN Session
	SessionManager->CreateSession(PlayerName, MaxPlayers);
}

void UPP_HostSettingsWidget::OnCreateSessionCompleted(bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogPP, Verbose, TEXT("[HostSettings] Session created successfully, executing ServerTravel: %s"), *PendingTravelURL);

		// Session注册成功，执行 ServerTravel 开启 Listen Server
		APlayerController* PC = GetOwningPlayer();
		if (PC)
		{
			PC->ConsoleCommand(PendingTravelURL);
		}
		else
		{
			UE_LOG(LogPP, Error, TEXT("[HostSettings] No PlayerController available for ServerTravel!"));
		}
	}
	else
	{
		UE_LOG(LogPP, Error, TEXT("[HostSettings] Session creation failed! ServerTravel aborted."));
		// TODO: 可以在此显示错误提示给玩家
	}
}

void UPP_HostSettingsWidget::OnBackClicked()
{
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->HideHostSettings();
	}
}
