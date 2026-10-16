// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_LobbyWidget.h"
#include "Lobby/PP_LobbyHUD.h"
#include "Lobby/PP_PlayerEntryWidget.h"
#include "Lobby/PP_ChatMessageWidget.h"
#include "Framework/PP_UIStyle.h"
#include "Player/PP_PlayerController.h"
#include "GameFramework/PlayerState.h"
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

	const APlayerController* LocalPC = GetOwningPlayer();
	const APP_LobbyPlayerState* LocalPS = LocalPC ? LocalPC->GetPlayerState<APP_LobbyPlayerState>() : nullptr;
	const bool bLocalIsHost = LocalPS && LocalPS->IsHost();

	int32 ReadyCount = 0;
	for (const FLobbyPlayerInfo& PlayerInfo : Players)
	{
		if (PlayerInfo.bIsReady)
		{
			++ReadyCount;
		}
	}

	if (PlayerListContainer)
	{
		PlayerListContainer->ClearChildren();

		for (const FLobbyPlayerInfo& PlayerInfo : Players)
		{
			const bool bIsLocalEntry = LocalPS && (PlayerInfo.PlayerIndex == LocalPS->GetPlayerIndex());

			UPP_PlayerEntryWidget* EntryWidget = nullptr;
			if (PlayerEntryClass)
			{
				EntryWidget = Cast<UPP_PlayerEntryWidget>(CreateWidget<UUserWidget>(this, PlayerEntryClass));
				if (!EntryWidget)
				{
					UE_LOG(LogTemp, Warning,
						TEXT("[LobbyWidget] WBP_PlayerEntry 的父类不是 UPP_PlayerEntryWidget（请 Reparent），退回文本渲染"));
				}
			}
			else
			{
				UE_LOG(LogTemp, Error,
					TEXT("[LobbyWidget] PlayerEntryClass 未配置：请在 WBP_Lobby 的 Class Defaults 里指向 WBP_PlayerEntry"));
			}

			if (EntryWidget)
			{
				EntryWidget->SetPlayerInfo(PlayerInfo, bIsLocalEntry);
				PlayerListContainer->AddChild(EntryWidget);
			}
			else
			{
				PlayerListContainer->AddChild(CreateFallbackPlayerText(PlayerInfo));
			}
		}

		// 空位补齐到 MaxPlayers，避免列表看着残缺（MaxPlayers 改到 8 时容器需套 ScrollBox）
		for (int32 SlotIndex = Players.Num(); SlotIndex < CachedMaxPlayers; ++SlotIndex)
		{
			UPP_PlayerEntryWidget* EntryWidget = nullptr;
			if (PlayerEntryClass)
			{
				EntryWidget = Cast<UPP_PlayerEntryWidget>(CreateWidget<UUserWidget>(this, PlayerEntryClass));
			}

			if (EntryWidget)
			{
				EntryWidget->SetEmptySlot(SlotIndex);
				PlayerListContainer->AddChild(EntryWidget);
			}
			else
			{
				UTextBlock* SlotText = NewObject<UTextBlock>(this);
				if (SlotText)
				{
					SlotText->SetText(FText::FromString(TEXT("+ \u7a7a\u4f4d  \u7b49\u5f85\u52a0\u5165")));
					SlotText->SetColorAndOpacity(FSlateColor(UPP_UIStyle::GetDimColor()));
					PlayerListContainer->AddChild(SlotText);
				}
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[LobbyWidget] PlayerListContainer is NULL!"));
	}

	ApplyHostOnlyButtons(bLocalIsHost);
	UpdateBottomBar(ReadyCount, Players.Num(), bLocalIsHost);

	BP_OnPlayerListUpdated(Players);
}

UTextBlock* UPP_LobbyWidget::CreateFallbackPlayerText(const FLobbyPlayerInfo& PlayerInfo)
{
	UTextBlock* EntryText = NewObject<UTextBlock>(this);
	if (!EntryText)
	{
		return nullptr;
	}

	FString StatusText = PlayerInfo.bIsReady ? TEXT("\u5df2\u51c6\u5907") : TEXT("\u672a\u51c6\u5907");
	FString ClassName = PlayerInfo.SelectedCharacterId.IsNone() ? TEXT("\u672a\u9009\u62e9") : PlayerInfo.SelectedCharacterId.ToString();
	FString HostPrefix = PlayerInfo.bIsHost ? TEXT("[\u623f\u4e3b] ") : TEXT("");
	FString DisplayText = FString::Printf(TEXT("%s%s (%s)    [%s]"), *HostPrefix, *PlayerInfo.PlayerName, *ClassName, *StatusText);
	EntryText->SetText(FText::FromString(DisplayText));

	if (PlayerInfo.bIsHost)
	{
		EntryText->SetColorAndOpacity(FSlateColor(UPP_UIStyle::GetGoldColor()));
	}
	else if (PlayerInfo.bIsReady)
	{
		EntryText->SetColorAndOpacity(FSlateColor(UPP_UIStyle::GetReadyColor()));
	}
	else
	{
		EntryText->SetColorAndOpacity(FSlateColor(UPP_UIStyle::GetTextColor()));
	}

	return EntryText;
}

void UPP_LobbyWidget::ApplyRoomSettings(const FRoomSettings& Settings)
{
	CachedMaxPlayers = FMath::Max(1, Settings.MaxPlayers);

	if (RoomMetaText)
	{
		RoomMetaText->SetText(FText::FromString(
			FString::Printf(TEXT("%s \u00b7 %s"), *Settings.MapName, *Settings.GameModeName)));
	}

	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), LastRenderedPlayerCount, CachedMaxPlayers)));
	}
}

void UPP_LobbyWidget::SetMigrationBannerVisible(bool bVisible)
{
	if (MigrationBanner)
	{
		MigrationBanner->SetVisibility(bVisible ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UPP_LobbyWidget::ApplyHostOnlyButtons(bool bLocalIsHost)
{
	// 关键：保持 Visible 占位，只切 IsEnabled。
	// 之前用 Collapsed 会导致非房主玩家看到的底栏按钮整排左移。
	if (StartButton)
	{
		StartButton->SetVisibility(ESlateVisibility::Visible);
		StartButton->SetIsEnabled(bLocalIsHost);
	}
	if (RoomSettingsButton)
	{
		RoomSettingsButton->SetVisibility(ESlateVisibility::Visible);
		RoomSettingsButton->SetIsEnabled(bLocalIsHost);
	}
}

void UPP_LobbyWidget::UpdateBottomBar(int32 ReadyCount, int32 TotalPlayers, bool bLocalIsHost)
{
	LastRenderedPlayerCount = TotalPlayers;

	if (PlayerCountText)
	{
		PlayerCountText->SetText(FText::FromString(
			FString::Printf(TEXT("%d / %d"), TotalPlayers, CachedMaxPlayers)));
	}

	if (ReadyCountText)
	{
		FString Text = FString::Printf(TEXT("\u5df2\u51c6\u5907 %d / %d"), ReadyCount, TotalPlayers);
		if (ReadyCount < TotalPlayers)
		{
			Text += TEXT(" \u00b7 \u7b49\u5f85\u5176\u4ed6\u73a9\u5bb6");
		}
		ReadyCountText->SetText(FText::FromString(Text));
	}

	// 开始条件：房主 + 有人 + 全员已准备
	const bool bCanStart = bLocalIsHost && TotalPlayers > 0 && ReadyCount >= TotalPlayers;
	if (StartButton)
	{
		StartButton->SetIsEnabled(bCanStart);
	}

	if (StartBlockReasonText)
	{
		if (bCanStart)
		{
			StartBlockReasonText->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			FString Reason;
			if (!bLocalIsHost)
			{
				Reason = TEXT("\u4ec5\u623f\u4e3b\u53ef\u7528");
			}
			else if (TotalPlayers == 0)
			{
				Reason = TEXT("\u7b49\u5f85\u73a9\u5bb6\u52a0\u5165");
			}
			else
			{
				Reason = FString::Printf(TEXT("\u8fd8\u6709 %d \u4eba\u672a\u51c6\u5907"), TotalPlayers - ReadyCount);
			}
			StartBlockReasonText->SetText(FText::FromString(Reason));
			StartBlockReasonText->SetVisibility(ESlateVisibility::Visible);
		}
	}
}

bool UPP_LobbyWidget::IsChatScrolledToBottom() const
{
	if (!ChatDisplayArea) return true;

	const float CurrentOffset = ChatDisplayArea->GetScrollOffset();
	const float EndOffset = ChatDisplayArea->GetScrollOffsetOfEnd();
	return (EndOffset - CurrentOffset) < 5.f;
}

void UPP_LobbyWidget::UpdateCountdown(float InDeltaTime)
{
	if (CountdownRemaining < 0.f) return;

	CountdownRemaining = FMath::Max(0.f, CountdownRemaining - InDeltaTime);

	if (CountdownBigText)
	{
		CountdownBigText->SetText(FText::FromString(
			FString::Printf(TEXT("%d"), FMath::CeilToInt(CountdownRemaining))));
	}
	if (CountdownChipText)
	{
		CountdownChipText->SetText(FText::FromString(
			FString::Printf(TEXT("\u23f1 %.1fs"), CountdownRemaining)));
	}
	if (CountdownOverlay)
	{
		CountdownOverlay->SetVisibility(ESlateVisibility::Visible);
	}
}

void UPP_LobbyWidget::AddChatMessage(const FString& PlayerName, const FString& Message)
{
	if (!ChatDisplayArea) return;

	// 用户往上翻看历史时，新消息不要把视图强行拉回底部
	const bool bWasAtBottom = IsChatScrolledToBottom();

	const APlayerController* PC = GetOwningPlayer();
	const APlayerState* PS = PC ? PC->GetPlayerState<APlayerState>() : nullptr;
	const bool bIsLocalPlayer = PS && PS->GetPlayerName() == PlayerName;

	if (ChatMessageClass)
	{
		UPP_ChatMessageWidget* MsgWidget = CreateWidget<UPP_ChatMessageWidget>(this, ChatMessageClass);
		if (MsgWidget)
		{
			MsgWidget->SetChatMessage(PlayerName, Message, bIsLocalPlayer);
			ChatDisplayArea->AddChild(MsgWidget);
			if (bWasAtBottom) ChatDisplayArea->ScrollToEnd();
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[LobbyWidget] ChatMessageClass 未配置：请在 WBP_Lobby 的 Class Defaults 里指向 WBP_ChatMessage"));
	}

	// 回退：未配置 ChatMessageClass 时走旧写法
	UTextBlock* ChatEntry = NewObject<UTextBlock>(this);
	if (ChatEntry)
	{
		ChatEntry->SetText(FText::FromString(FString::Printf(TEXT("%s: %s"), *PlayerName, *Message)));
		ChatDisplayArea->AddChild(ChatEntry);
		if (bWasAtBottom) ChatDisplayArea->ScrollToEnd();
	}
}

void UPP_LobbyWidget::AddSystemMessage(const FString& Message)
{
	if (!ChatDisplayArea) return;

	const bool bWasAtBottom = IsChatScrolledToBottom();

	if (ChatMessageClass)
	{
		UPP_ChatMessageWidget* MsgWidget = CreateWidget<UPP_ChatMessageWidget>(this, ChatMessageClass);
		if (MsgWidget)
		{
			MsgWidget->SetSystemMessage(Message);
			ChatDisplayArea->AddChild(MsgWidget);
			if (bWasAtBottom) ChatDisplayArea->ScrollToEnd();
			return;
		}
	}
	else
	{
		UE_LOG(LogTemp, Error,
			TEXT("[LobbyWidget] ChatMessageClass 未配置：系统消息无法使用 WBP_ChatMessage 样式"));
	}

	UTextBlock* SysEntry = NewObject<UTextBlock>(this);
	if (SysEntry)
	{
		SysEntry->SetText(FText::FromString(FString::Printf(TEXT("\u2014 %s \u2014"), *Message)));
		SysEntry->SetColorAndOpacity(FSlateColor(UPP_UIStyle::GetGoldColor()));
		ChatDisplayArea->AddChild(SysEntry);
		if (bWasAtBottom) ChatDisplayArea->ScrollToEnd();
	}
}

void UPP_LobbyWidget::OnGameStarting(float Countdown)
{
	CountdownRemaining = Countdown;
	BP_OnGameStarting(Countdown);
}

void UPP_LobbyWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 倒计时：驱动中央覆盖层与底栏小字
	UpdateCountdown(InDeltaTime);

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
