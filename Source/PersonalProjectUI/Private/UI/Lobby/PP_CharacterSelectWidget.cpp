// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_CharacterSelectWidget.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "Game/PP_DataManager.h"
#include "Player/PP_PlayerController.h"
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
	APP_PlayerController* PC = GetOwningPlayer<APP_PlayerController>();
	if (PC) PC->Server_SelectCharacter(CurrentlySelected);
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

APP_LobbyHUD* UPP_CharacterSelectWidget::GetLobbyHUD() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<APP_LobbyHUD>(PC->GetHUD()) : nullptr;
}

void UPP_CharacterSelectWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!CharacterPreviewImage) return;

	if (APP_LobbyHUD* HUD = GetLobbyHUD())
	{
		if (UTextureRenderTarget2D* RT = HUD->GetPreviewRenderTarget())
		{
			if (!bPreviewImageReady || CharacterPreviewImage->GetBrush().GetResourceObject() != RT)
			{
				FSlateBrush NewBrush;
				NewBrush.SetResourceObject(RT);
				NewBrush.ImageSize = FVector2D(RT->SizeX, RT->SizeY);
				CharacterPreviewImage->SetBrush(NewBrush);
				bPreviewImageReady = true;
			}
		}
	}
}

FReply UPP_CharacterSelectWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && CharacterPreviewImage)
	{
		const FGeometry ImageGeometry = CharacterPreviewImage->GetCachedGeometry();
		if (ImageGeometry.IsUnderLocation(InMouseEvent.GetScreenSpacePosition()))
		{
			bIsDraggingPreview = true;
			LastDragMousePosition = InMouseEvent.GetScreenSpacePosition();
			return FReply::Handled().CaptureMouse(TakeWidget());
		}
	}
	return FReply::Unhandled();
}

FReply UPP_CharacterSelectWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (bIsDraggingPreview)
	{
		const FVector2D CurrentPosition = InMouseEvent.GetScreenSpacePosition();
		const FVector2D Delta = CurrentPosition - LastDragMousePosition;
		LastDragMousePosition = CurrentPosition;

		if (APP_LobbyHUD* HUD = GetLobbyHUD())
		{
			HUD->RotatePreviewCharacter(Delta.X * 0.2f, -Delta.Y * 0.2f);
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

FReply UPP_CharacterSelectWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingPreview)
	{
		bIsDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

