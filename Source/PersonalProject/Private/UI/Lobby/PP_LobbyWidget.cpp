// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_LobbyWidget.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "Player/PP_PlayerController.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Game/PP_SessionManager.h"
#include "Engine/GameInstance.h"
#include "Engine/TextureRenderTarget2D.h"
#include "UObject/UnrealType.h"

void UPP_LobbyWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 如果PlayerEntryClass未在蓝图中设置，加载默认WBP_PlayerEntry蓝图
	if (!PlayerEntryClass)
	{
		PlayerEntryClass = LoadClass<UUserWidget>(nullptr, TEXT("/Game/PersonalProject/UI/LobbyInterface/Components/WBP_PlayerEntry.WBP_PlayerEntry_C"));
		if (!PlayerEntryClass)
		{
			UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] Failed to load WBP_PlayerEntry class"));
		}
	}
	if (ReadyButton) ReadyButton->OnClicked.AddDynamic(this, &UPP_LobbyWidget::OnReadyClicked);
	if (StartButton) StartButton->OnClicked.AddDynamic(this, &UPP_LobbyWidget::OnStartClicked);
	if (CharacterSelectButton) CharacterSelectButton->OnClicked.AddDynamic(this, &UPP_LobbyWidget::OnCharacterSelectClicked);
	if (RoomSettingsButton) RoomSettingsButton->OnClicked.AddDynamic(this, &UPP_LobbyWidget::OnRoomSettingsClicked);
	if (LeaveButton) LeaveButton->OnClicked.AddDynamic(this, &UPP_LobbyWidget::OnLeaveClicked);
	if (ChatInputBox) ChatInputBox->OnTextCommitted.AddDynamic(this, &UPP_LobbyWidget::OnChatTextCommitted);
}

void UPP_LobbyWidget::UpdatePlayerList(const TArray<FLobbyPlayerInfo>& Players)
{
	UWorld* World = GetWorld();
	if (!World || World->bIsTearingDown) return;

	// C++ 直接渲染玩家列表（不依赖蓝图事件）
	if (PlayerListContainer)
	{
		PlayerListContainer->ClearChildren();

		for (const FLobbyPlayerInfo& PlayerInfo : Players)
		{
			UTextBlock* EntryText = NewObject<UTextBlock>(this);
			if (EntryText)
			{
				// 格式："[房主] 玩家名 (职业)    [已准备/未准备]"
				FString StatusText = PlayerInfo.bIsReady ? TEXT("\u5df2\u51c6\u5907") : TEXT("\u672a\u51c6\u5907");
				FString ClassName = PlayerInfo.SelectedCharacterId.IsNone() ? TEXT("\u672a\u9009\u62e9") : PlayerInfo.SelectedCharacterId.ToString();
				FString HostPrefix = PlayerInfo.bIsHost ? TEXT("[\u623f\u4e3b] ") : TEXT("");
				FString DisplayText = FString::Printf(TEXT("%s%s (%s)    [%s]"), *HostPrefix, *PlayerInfo.PlayerName, *ClassName, *StatusText);
				EntryText->SetText(FText::FromString(DisplayText));

				// 设置颜色：金色=房主，绿色=已准备，白色=未准备
				if (PlayerInfo.bIsHost)
				{
					EntryText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 0.84f, 0.0f, 1.0f)));
				}
				else if (PlayerInfo.bIsReady)
				{
					EntryText->SetColorAndOpacity(FSlateColor(FLinearColor(0.0f, 1.0f, 0.0f, 1.0f)));
				}
				else
				{
					EntryText->SetColorAndOpacity(FSlateColor(FLinearColor(1.0f, 1.0f, 1.0f, 1.0f)));
				}

				// 设置字体大小
				FSlateFontInfo FontInfo = EntryText->GetFont();
				FontInfo.Size = 18;
				EntryText->SetFont(FontInfo);

				PlayerListContainer->AddChild(EntryText);
			}
		}

	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyWidget] PlayerListContainer is NULL!"));
	}

	// 控制房主专属按钮可见性
	APlayerController* LocalPC = GetOwningPlayer();
	APP_LobbyPlayerState* LocalPS = LocalPC ? LocalPC->GetPlayerState<APP_LobbyPlayerState>() : nullptr;
	bool bLocalIsHost = LocalPS && LocalPS->IsHost();

	if (StartButton)
	{
		StartButton->SetVisibility(bLocalIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (RoomSettingsButton)
	{
		RoomSettingsButton->SetVisibility(bLocalIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPP_LobbyWidget::AddChatMessage(const FString& PlayerName, const FString& Message)
{
	if (!ChatDisplayArea) return;

	UTextBlock* ChatEntry = NewObject<UTextBlock>(this);
	if (ChatEntry)
	{
		ChatEntry->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *PlayerName, *Message)));
		ChatDisplayArea->AddChild(ChatEntry);
		ChatDisplayArea->ScrollToEnd();
	}
}

void UPP_LobbyWidget::OnGameStarting(float Countdown) { BP_OnGameStarting(Countdown); }

void UPP_LobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
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

FReply UPP_LobbyWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply UPP_LobbyWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
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

FReply UPP_LobbyWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && bIsDraggingPreview)
	{
		bIsDraggingPreview = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

void UPP_LobbyWidget::OnReadyClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	APP_PlayerController* LobbyPC = Cast<APP_PlayerController>(PC);
	if (!LobbyPC) return;

	// 从 PlayerState 获取当前准备状态并反转
	APP_LobbyPlayerState* PS = PC->GetPlayerState<APP_LobbyPlayerState>();
	bool bCurrentReady = PS ? PS->IsReady() : false;
	LobbyPC->Server_SetReady(!bCurrentReady);
}

void UPP_LobbyWidget::OnStartClicked()
{
	APP_PlayerController* PC = GetOwningPlayer<APP_PlayerController>();
	if (PC) PC->Server_StartGame();
}

void UPP_LobbyWidget::OnCharacterSelectClicked()
{
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->ShowCharacterSelect();
	}
}

void UPP_LobbyWidget::OnRoomSettingsClicked()
{
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->ShowRoomSettings();
	}
}

void UPP_LobbyWidget::OnLeaveClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (!PC) return;

	APP_LobbyPlayerState* PS = PC->GetPlayerState<APP_LobbyPlayerState>();

	if (PS && PS->IsHost())
	{
		// 房主离开 - 触发迁移（不立即断开，由服务端延迟处理）
		APP_PlayerController* LobbyPC = Cast<APP_PlayerController>(PC);
		if (LobbyPC)
		{
			UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Host leaving - requesting host migration"));
			LobbyPC->Server_RequestHostMigration();
		}
	}
	else
	{
		// 非房主离开 - 正常断开
		UE_LOG(LogTemp, Log, TEXT("[LobbyWidget] Non-host leaving - normal disconnect"));

		UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
		if (SM)
		{
			SM->DestroyCurrentSession();
		}

		UWorld* World = GetWorld();
		if (World)
		{
			World->GetTimerManager().SetTimer(
				LeaveTimerHandle, this, &UPP_LobbyWidget::ExecuteDisconnect, 0.2f, false);
		}
		else
		{
			ExecuteDisconnect();
		}
	}
}

void UPP_LobbyWidget::ExecuteDisconnect()
{
	UE_LOG(LogTemp, Warning, TEXT("[LobbyWidget] ExecuteDisconnect - performing Disconnect"));
	APlayerController* PC = GetOwningPlayer();
	if (PC)
	{
		PC->ConsoleCommand(TEXT("Disconnect"));
	}
}

APP_LobbyHUD* UPP_LobbyWidget::GetLobbyHUD() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<APP_LobbyHUD>(PC->GetHUD()) : nullptr;
}

void UPP_LobbyWidget::OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod)
{
	if (CommitMethod == ETextCommit::OnEnter)
	{
		FString Message = Text.ToString();
		if (!Message.IsEmpty())
		{
			APP_PlayerController* PC = GetOwningPlayer<APP_PlayerController>();
			if (PC) PC->Server_SendChatMessage(Message);
			if (ChatInputBox) ChatInputBox->SetText(FText::GetEmpty());
		}
	}
}
