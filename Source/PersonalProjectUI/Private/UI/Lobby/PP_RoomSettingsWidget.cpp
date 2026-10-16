// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_RoomSettingsWidget.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "Player/PP_PlayerController.h"

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
	APP_PlayerController* PC = GetOwningPlayer<APP_PlayerController>();
	if (PC) PC->Server_UpdateRoomSettings(NewSettings);
}

void UPP_RoomSettingsWidget::OnCloseClicked()
{
	APP_LobbyHUD* LobbyHUD = GetOwningPlayer() ? Cast<APP_LobbyHUD>(GetOwningPlayer()->GetHUD()) : nullptr;
	if (LobbyHUD) LobbyHUD->HideRoomSettings();
}
