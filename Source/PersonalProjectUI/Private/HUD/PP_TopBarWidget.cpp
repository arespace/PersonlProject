// Chen Tianbao uses GAS to create online battle games

#include "HUD/PP_TopBarWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Components/Button.h"
#include "GameFramework/PlayerController.h"
#include "Framework/PP_Log.h"

void UPP_TopBarWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	if (NavButton_MainMenu)        NavButton_MainMenu->OnClicked.AddDynamic(this, &UPP_TopBarWidget::OnMainMenuClicked);
	if (NavButton_Lobby)           NavButton_Lobby->OnClicked.AddDynamic(this, &UPP_TopBarWidget::OnLobbyClicked);
	if (NavButton_CharacterSelect) NavButton_CharacterSelect->OnClicked.AddDynamic(this, &UPP_TopBarWidget::OnCharacterSelectClicked);
	if (NavButton_Settings)        NavButton_Settings->OnClicked.AddDynamic(this, &UPP_TopBarWidget::OnSettingsClicked);

	// 还在房间里时「主菜单」按钮置灰：必须先退出房间才能回主菜单
	if (NavButton_MainMenu)
	{
		if (const APP_LobbyHUD* HUD = GetLobbyHUD())
		{
			NavButton_MainMenu->SetIsEnabled(!HUD->IsInRoom());
		}
	}
}

void UPP_TopBarWidget::OnMainMenuClicked()
{
	APP_LobbyHUD* HUD = GetLobbyHUD();
	if (!HUD) return;

	// 还在房间里 → 不允许前往主菜单（按钮此时也应该是置灰的）
	if (HUD->IsInRoom())
	{
		UE_LOG(LogPP, Warning, TEXT("[TopBar] 还在房间中，请先离开房间再返回主菜单"));
		return;
	}

	HUD->ShowMainMenu();
}

void UPP_TopBarWidget::OnLobbyClicked()
{
	if (APP_LobbyHUD* HUD = GetLobbyHUD())
	{
		HUD->ShowLobby();
	}
}

void UPP_TopBarWidget::OnCharacterSelectClicked()
{
	if (APP_LobbyHUD* HUD = GetLobbyHUD())
	{
		HUD->ShowCharacterSelect();
	}
}

void UPP_TopBarWidget::OnSettingsClicked()
{
	if (APP_LobbyHUD* HUD = GetLobbyHUD())
	{
		HUD->ShowSettings();
	}
}

APP_LobbyHUD* UPP_TopBarWidget::GetLobbyHUD() const
{
	const APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<APP_LobbyHUD>(PC->GetHUD()) : nullptr;
}
