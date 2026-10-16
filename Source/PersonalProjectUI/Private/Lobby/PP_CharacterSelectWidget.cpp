// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_CharacterSelectWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_LobbyActions.h"
#include "GameFramework/PlayerController.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"

void UPP_CharacterSelectWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();
	if (ConfirmButton) ConfirmButton->OnClicked.AddDynamic(this, &UPP_CharacterSelectWidget::OnConfirmClicked);
	if (BackButton) BackButton->OnClicked.AddDynamic(this, &UPP_CharacterSelectWidget::OnBackClicked);
	if (PrevCharacterButton) PrevCharacterButton->OnClicked.AddDynamic(this, &UPP_CharacterSelectWidget::OnPrevCharacterClicked);
	if (NextCharacterButton) NextCharacterButton->OnClicked.AddDynamic(this, &UPP_CharacterSelectWidget::OnNextCharacterClicked);

	// 优先从 DataManager 全局角色表读取行名；没有则回退到默认角色列表
	if (UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld()))
	{
		AvailableCharacters = DataManager->GetCharacterRowNames();
	}
	if (AvailableCharacters.Num() == 0)
	{
		AvailableCharacters.Add(FName("Warrior"));
		AvailableCharacters.Add(FName("Archer"));
	}

	// 默认选中 Warrior（第一个角色）
	CurrentIndex = 0;
	if (AvailableCharacters.Num() > 0)
	{
		SetSelectedCharacter(AvailableCharacters[CurrentIndex]);
	}
}

void UPP_CharacterSelectWidget::OnConfirmClicked()
{
	if (CurrentlySelected.IsNone()) return;
	// 经 IPP_LobbyActions 接口请求选角，避免 UI 模块 include 主玩法 PlayerController 头文件
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (IPP_LobbyActions* Actions = Cast<IPP_LobbyActions>(PC))
		{
			IPP_LobbyActions::Execute_RequestSelectCharacter(PC, CurrentlySelected);
		}
	}
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD()) LobbyHUD->HideCharacterSelect();
}

void UPP_CharacterSelectWidget::SetSelectedCharacter(FName CharacterId)
{
	CurrentlySelected = CharacterId;

	// 详情面板：优先用表里的 DisplayName / Description（统一走 DataManager 全局角色表）
	bool bFilledFromTable = false;
	if (UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld()))
	{
		if (const FCharacterDataRow* Row = DataManager->GetCharacterData(CharacterId))
		{
			if (CharacterNameText)
			{
				CharacterNameText->SetText(Row->DisplayName);
			}
			if (CharacterDescText)
			{
				CharacterDescText->SetText(Row->Description);
			}
			bFilledFromTable = true;
		}
	}

	if (!bFilledFromTable && CharacterNameText)
	{
		CharacterNameText->SetText(FText::FromName(CharacterId));
	}

	BP_OnCharacterSelected(CharacterId);
}

void UPP_CharacterSelectWidget::OnBackClicked()
{
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD()) LobbyHUD->HideCharacterSelect();
}

void UPP_CharacterSelectWidget::OnPrevCharacterClicked()
{
	if (AvailableCharacters.Num() == 0) return;
	CurrentIndex = (CurrentIndex - 1 + AvailableCharacters.Num()) % AvailableCharacters.Num();
	SetSelectedCharacter(AvailableCharacters[CurrentIndex]);
	NotifyPreviewUpdate(AvailableCharacters[CurrentIndex]);
}

void UPP_CharacterSelectWidget::OnNextCharacterClicked()
{
	if (AvailableCharacters.Num() == 0) return;
	CurrentIndex = (CurrentIndex + 1) % AvailableCharacters.Num();
	SetSelectedCharacter(AvailableCharacters[CurrentIndex]);
	NotifyPreviewUpdate(AvailableCharacters[CurrentIndex]);
}

void UPP_CharacterSelectWidget::NotifyPreviewUpdate(FName CharacterId)
{
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->UpdatePreviewCharacter(CharacterId);
	}
}


