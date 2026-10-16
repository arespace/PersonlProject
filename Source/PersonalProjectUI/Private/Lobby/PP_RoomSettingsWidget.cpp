// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_RoomSettingsWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Framework/PP_LobbyActions.h"
#include "GameFramework/PlayerController.h"

void UPP_RoomSettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (MapSelector) { MapSelector->AddOption(TEXT("PPMap")); MapSelector->SetSelectedOption(TEXT("PPMap")); }
	if (MaxPlayersSpinBox) { MaxPlayersSpinBox->SetMinValue(2.f); MaxPlayersSpinBox->SetMaxValue(16.f); MaxPlayersSpinBox->SetValue(4.f); }
	if (GameModeSelector) { GameModeSelector->AddOption(TEXT("Crash")); GameModeSelector->SetSelectedOption(TEXT("Crash")); }
	if (ApplyButton) ApplyButton->OnClicked.AddDynamic(this, &UPP_RoomSettingsWidget::OnApplyClicked);
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UPP_RoomSettingsWidget::OnCloseClicked);
}

void UPP_RoomSettingsWidget::ApplySettings(const FRoomSettings& Settings)
{
	if (MapSelector) MapSelector->SetSelectedOption(Settings.MapName);
	if (MaxPlayersSpinBox) MaxPlayersSpinBox->SetValue(static_cast<double>(Settings.MaxPlayers));
	if (GameModeSelector) GameModeSelector->SetSelectedOption(Settings.GameModeName);
}

void UPP_RoomSettingsWidget::OnApplyClicked()
{
	FRoomSettings NewSettings;
	if (MapSelector) NewSettings.MapName = MapSelector->GetSelectedOption();
	if (MaxPlayersSpinBox) NewSettings.MaxPlayers = static_cast<int32>(MaxPlayersSpinBox->GetValue());
	if (GameModeSelector) NewSettings.GameModeName = GameModeSelector->GetSelectedOption();
	// 经 IPP_LobbyActions 接口提交房间设置，避免 UI 模块 include 主玩法 PlayerController 头文件
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (IPP_LobbyActions* Actions = Cast<IPP_LobbyActions>(PC))
		{
			IPP_LobbyActions::Execute_RequestUpdateRoomSettings(PC, NewSettings);
		}
	}
}

void UPP_RoomSettingsWidget::OnCloseClicked()
{
	APP_LobbyHUD* LobbyHUD = APP_LobbyHUD::GetFromWidget(this);
	if (LobbyHUD) LobbyHUD->HideRoomSettings();
}
