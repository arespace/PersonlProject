// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_MainMenuWidget.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "UI/PP_UIStyle.h"
#include "Game/PP_LocalSaveGame.h"
#include "GameFramework/PlayerController.h"
#include "Game/PP_SessionManager.h"
#include "Components/TextBlock.h"
#include "Engine/GameInstance.h"

void UPP_MainMenuWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// 原有按钮绑定
	if (HostButton) HostButton->OnClicked.AddDynamic(this, &UPP_MainMenuWidget::OnHostClicked);
	if (JoinButton) JoinButton->OnClicked.AddDynamic(this, &UPP_MainMenuWidget::OnJoinClicked);
	if (SettingsButton) SettingsButton->OnClicked.AddDynamic(this, &UPP_MainMenuWidget::OnSettingsClicked);
	if (QuitButton) QuitButton->OnClicked.AddDynamic(this, &UPP_MainMenuWidget::OnQuitClicked);

	// 房间列表相关绑定
	if (RefreshButton) RefreshButton->OnClicked.AddDynamic(this, &UPP_MainMenuWidget::OnRefreshButtonClicked);
	if (RoomListDropdown) RoomListDropdown->OnSelectionChanged.AddDynamic(this, &UPP_MainMenuWidget::OnRoomListSelectionChanged);

	// 从 GameInstance Subsystem 获取 SessionManager
	SessionManager = UPP_SessionManager::Get(GetWorld());
	if (SessionManager)
	{
		if (!SessionManager->OnSessionsFound.IsAlreadyBound(this, &UPP_MainMenuWidget::OnSessionsFound))
		{
			SessionManager->OnSessionsFound.AddDynamic(this, &UPP_MainMenuWidget::OnSessionsFound);
		}
		if (!SessionManager->OnJoinSessionComplete.IsAlreadyBound(this, &UPP_MainMenuWidget::OnJoinSessionComplete))
		{
			SessionManager->OnJoinSessionComplete.AddDynamic(this, &UPP_MainMenuWidget::OnJoinSessionComplete);
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[MainMenu] Failed to get SessionManager Subsystem!"));
	}

	// 版本号（右下角小字）
	if (VersionText)
	{
		VersionText->SetText(VersionLabel);
	}

	// 恢复上次使用的昵称
	if (PlayerNameInput)
	{
		if (UPP_LocalSaveGame* Save = UPP_LocalSaveGame::LoadOrCreate())
		{
			if (!Save->PlayerName.IsEmpty())
			{
				PlayerNameInput->SetText(FText::FromString(Save->PlayerName));
			}
		}
	}
}

void UPP_MainMenuWidget::NativeDestruct()
{
	// 在 Widget 销毁前解绑 SessionManager 委托，避免关卡切换时回调到已销毁的 Widget
	if (SessionManager)
	{
		SessionManager->OnSessionsFound.RemoveDynamic(this, &UPP_MainMenuWidget::OnSessionsFound);
		SessionManager->OnJoinSessionComplete.RemoveDynamic(this, &UPP_MainMenuWidget::OnJoinSessionComplete);
	}

	Super::NativeDestruct();
}

APP_LobbyHUD* UPP_MainMenuWidget::GetLobbyHUD() const
{
	APlayerController* PC = GetOwningPlayer();
	return PC ? Cast<APP_LobbyHUD>(PC->GetHUD()) : nullptr;
}

FString UPP_MainMenuWidget::GetPlayerName() const
{
	FString Name = PlayerNameInput ? PlayerNameInput->GetText().ToString() : TEXT("");
	Name.TrimStartAndEndInline();

	return Name.IsEmpty() ? TEXT("Player") : Name;
}

void UPP_MainMenuWidget::SavePlayerName(const FString& Name)
{
	if (Name.IsEmpty()) return;

	if (UPP_LocalSaveGame* Save = UPP_LocalSaveGame::LoadOrCreate())
	{
		Save->PlayerName = Name;
		UPP_LocalSaveGame::Save(Save);
	}
}

void UPP_MainMenuWidget::SetConnectingState(bool bConnecting)
{
	// 连接过程中禁用主操作按钮，避免重复点击
	if (JoinButton)    JoinButton->SetIsEnabled(!bConnecting);
	if (HostButton)    HostButton->SetIsEnabled(!bConnecting);
	if (RefreshButton) RefreshButton->SetIsEnabled(!bConnecting);

	if (bConnecting)
	{
		SetStatusMessage(TEXT("\u6b63\u5728\u8fde\u63a5\u2026"), false);
	}
	else
	{
		ClearStatusMessage();
	}
}

void UPP_MainMenuWidget::OnHostClicked()
{
	const FString Name = GetPlayerName();
	SavePlayerName(Name);

	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->SetPendingPlayerName(Name);
		LobbyHUD->ShowHostSettings();
	}
}

void UPP_MainMenuWidget::OnJoinClicked()
{
	const FString Name = GetPlayerName();
	SavePlayerName(Name);

	// 优先使用下拉框选中的房间加入
	if (SelectedSessionIndex >= 0 && SessionManager)
	{
		SetConnectingState(true);
		SessionManager->JoinSession(SelectedSessionIndex, Name);
		return;
	}

	// Fallback: 通过 IP 地址直连
	FString IPAddress = IPAddressInput ? IPAddressInput->GetText().ToString() : TEXT("");
	if (IPAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] No room selected and no IP address provided"));
		SetStatusMessage(TEXT("\u8bf7\u5148\u9009\u62e9\u4e00\u4e2a\u623f\u95f4\uff0c\u6216\u586b\u5199 IP \u5730\u5740"), true);
		return;
	}

	SetConnectingState(true);

	FString TravelURL = FString::Printf(TEXT("%s?PlayerName=%s"), *IPAddress, *Name);
	UE_LOG(LogTemp, Log, TEXT("[MainMenu] ClientTravel via IP: %s"), *TravelURL);
	APlayerController* PC = GetOwningPlayer();
	if (PC) PC->ClientTravel(TravelURL, TRAVEL_Absolute);
}

void UPP_MainMenuWidget::OnSettingsClicked()
{
	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->ShowSettings();
	}
}

void UPP_MainMenuWidget::OnQuitClicked()
{
	APlayerController* PC = GetOwningPlayer();
	if (PC) PC->ConsoleCommand(TEXT("Quit"));
}

void UPP_MainMenuWidget::RefreshRoomList()
{
	if (!SessionManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[MainMenu] SessionManager is null!"));
		return;
	}

	// 显示搜索中提示：优先用独立的状态文本（不会污染下拉框选项，也不会被当成房间选中）
	if (StatusText)
	{
		SetStatusMessage(TEXT("\u6b63\u5728\u641c\u7d22\u5c40\u57df\u7f51\u623f\u95f4\u2026"), false);
	}
	else if (RoomListDropdown)
	{
		RoomListDropdown->ClearOptions();
		RoomListDropdown->AddOption(TEXT("\u6b63\u5728\u641c\u7d22..."));
		RoomListDropdown->SetSelectedOption(TEXT("\u6b63\u5728\u641c\u7d22..."));
	}

	SelectedSessionIndex = -1;
	SessionManager->FindSessions();
}

void UPP_MainMenuWidget::OnRefreshButtonClicked()
{
	RefreshRoomList();
}

void UPP_MainMenuWidget::OnSessionsFound(const TArray<FRoomInfo>& Rooms)
{
	UE_LOG(LogTemp, Log, TEXT("[MainMenu] Sessions found: %d"), Rooms.Num());

	if (!RoomListDropdown) return;

	RoomListDropdown->ClearOptions();
	SelectedSessionIndex = -1;

	if (Rooms.Num() == 0)
	{
		if (StatusText)
		{
			SetStatusMessage(TEXT("\u672a\u627e\u5230\u53ef\u7528\u623f\u95f4\uff0c\u70b9\u51fb\u5237\u65b0\u91cd\u8bd5"), true);
		}
		else
		{
			RoomListDropdown->AddOption(TEXT("\u6682\u65e0\u53ef\u7528\u623f\u95f4"));
			RoomListDropdown->SetSelectedOption(TEXT("\u6682\u65e0\u53ef\u7528\u623f\u95f4"));
		}
		return;
	}

	// 搜索到结果，清掉「正在搜索…」状态
	ClearStatusMessage();

	for (int32 i = 0; i < Rooms.Num(); i++)
	{
		const FRoomInfo& Room = Rooms[i];
		// 格式: "房间名 (当前人数/最大人数) Ping:XXms"
		FString DisplayText = FString::Printf(TEXT("%s (%d/%d) Ping:%dms"),
			*Room.RoomName, Room.CurrentPlayers, Room.MaxPlayers, Room.Ping);
		RoomListDropdown->AddOption(DisplayText);
	}

	// 默认选中第一个房间
	RoomListDropdown->SetSelectedIndex(0);
	SelectedSessionIndex = 0;
}

void UPP_MainMenuWidget::OnRoomListSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType)
{
	if (!RoomListDropdown) return;

	// 获取选中的索引
	int32 Index = RoomListDropdown->GetSelectedIndex();

	// 检查是否是提示项（非有效房间）
	if (SelectedItem == TEXT("\u6b63\u5728\u641c\u7d22...") || SelectedItem == TEXT("\u6682\u65e0\u53ef\u7528\u623f\u95f4"))
	{
		SelectedSessionIndex = -1;
		return;
	}

	SelectedSessionIndex = Index;
	UE_LOG(LogTemp, Log, TEXT("[MainMenu] Room selected: %s (Index: %d)"), *SelectedItem, SelectedSessionIndex);
}

void UPP_MainMenuWidget::OnJoinSessionComplete(bool bSuccess)
{
	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("[MainMenu] Successfully joined session! ClientTravel handled by SessionManager."));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] Failed to join session."));

		// 恢复按钮，允许重新选房间或重新刷新
		SetConnectingState(false);

		if (StatusText)
		{
			SetStatusMessage(TEXT("\u52a0\u5165\u623f\u95f4\u5931\u8d25\uff1a\u4f1a\u8bdd\u5df2\u5173\u95ed\uff0c\u8bf7\u91cd\u65b0\u5237\u65b0"), true);
		}
		else if (RoomListDropdown)
		{
			// 刷新房间列表以获取最新状态
			RoomListDropdown->ClearOptions();
			RoomListDropdown->AddOption(TEXT("\u52a0\u5165\u5931\u8d25\uff0c\u8bf7\u91cd\u65b0\u5237\u65b0"));
			RoomListDropdown->SetSelectedOption(TEXT("\u52a0\u5165\u5931\u8d25\uff0c\u8bf7\u91cd\u65b0\u5237\u65b0"));
		}
		SelectedSessionIndex = -1;
	}
}

void UPP_MainMenuWidget::SetStatusMessage(const FString& Message, bool bIsError)
{
	if (!StatusText) return;

	StatusText->SetText(FText::FromString(Message));
	StatusText->SetColorAndOpacity(FSlateColor(bIsError ? UPP_UIStyle::GetDangerColor() : UPP_UIStyle::GetMutedColor()));
	StatusText->SetVisibility(ESlateVisibility::Visible);

	// 错误提示停留 5s，普通提示 30s 后自动收起
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().SetTimer(
			StatusClearTimerHandle, this, &UPP_MainMenuWidget::ClearStatusMessage, bIsError ? 5.f : 30.f, false);
	}
}

void UPP_MainMenuWidget::ClearStatusMessage()
{
	if (!StatusText) return;

	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(StatusClearTimerHandle);
	}
	StatusText->SetVisibility(ESlateVisibility::Collapsed);
}