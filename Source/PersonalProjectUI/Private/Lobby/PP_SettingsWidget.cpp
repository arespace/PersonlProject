// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_SettingsWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "GameFramework/GameUserSettings.h"

void UPP_SettingsWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ResolutionComboBox)
	{
		ResolutionComboBox->AddOption(TEXT("1920x1080"));
		ResolutionComboBox->AddOption(TEXT("1280x720"));
		ResolutionComboBox->SetSelectedOption(TEXT("1920x1080"));
	}
	if (WindowModeComboBox)
	{
		WindowModeComboBox->AddOption(TEXT("Fullscreen"));
		WindowModeComboBox->AddOption(TEXT("Windowed"));
		WindowModeComboBox->AddOption(TEXT("Borderless"));
		WindowModeComboBox->SetSelectedOption(TEXT("Fullscreen"));
	}
	if (ApplyButton) ApplyButton->OnClicked.AddDynamic(this, &UPP_SettingsWidget::OnApplyClicked);
	if (ResetButton) ResetButton->OnClicked.AddDynamic(this, &UPP_SettingsWidget::OnResetClicked);
	if (CloseButton) CloseButton->OnClicked.AddDynamic(this, &UPP_SettingsWidget::OnCloseClicked);
}

void UPP_SettingsWidget::OnApplyClicked()
{
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (!Settings) return;
	if (ResolutionComboBox)
	{
		FString ResStr = ResolutionComboBox->GetSelectedOption();
		int32 Width = 1920, Height = 1080;
		TArray<FString> Parts;
		ResStr.ParseIntoArray(Parts, TEXT("x"));
		if (Parts.Num() == 2) { Width = FCString::Atoi(*Parts[0]); Height = FCString::Atoi(*Parts[1]); }
		Settings->SetScreenResolution(FIntPoint(Width, Height));
	}
	if (WindowModeComboBox)
	{
		FString Mode = WindowModeComboBox->GetSelectedOption();
		if (Mode == TEXT("Fullscreen")) Settings->SetFullscreenMode(EWindowMode::Fullscreen);
		else if (Mode == TEXT("Windowed")) Settings->SetFullscreenMode(EWindowMode::Windowed);
		else Settings->SetFullscreenMode(EWindowMode::WindowedFullscreen);
	}
	Settings->ApplySettings(false);
}

void UPP_SettingsWidget::OnResetClicked()
{
	UGameUserSettings* Settings = GEngine->GetGameUserSettings();
	if (Settings) { Settings->ApplySettings(true); }
	if (MasterVolumeSlider) MasterVolumeSlider->SetValue(1.f);
	if (SFXVolumeSlider) SFXVolumeSlider->SetValue(1.f);
	if (MusicVolumeSlider) MusicVolumeSlider->SetValue(1.f);
	if (ResolutionComboBox) ResolutionComboBox->SetSelectedOption(TEXT("1920x1080"));
	if (WindowModeComboBox) WindowModeComboBox->SetSelectedOption(TEXT("Fullscreen"));
}

void UPP_SettingsWidget::OnCloseClicked()
{
	APP_LobbyHUD* LobbyHUD = GetOwningPlayer() ? Cast<APP_LobbyHUD>(GetOwningPlayer()->GetHUD()) : nullptr;
	if (LobbyHUD) LobbyHUD->HideSettings();
}
