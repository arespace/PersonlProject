// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_MainMenuWidget.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "GameFramework/PlayerController.h"
#include "Game/PP_SessionManager.h"
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

void UPP_MainMenuWidget::OnHostClicked()
{
	FString Name = PlayerNameInput ? PlayerNameInput->GetText().ToString() : TEXT("");
	if (Name.IsEmpty()) Name = TEXT("Player");

	if (APP_LobbyHUD* LobbyHUD = GetLobbyHUD())
	{
		LobbyHUD->SetPendingPlayerName(Name);
		LobbyHUD->ShowHostSettings();
	}
}

void UPP_MainMenuWidget::OnJoinClicked()
{
	FString Name = PlayerNameInput ? PlayerNameInput->GetText().ToString() : TEXT("");
	if (Name.IsEmpty()) Name = TEXT("Player");

	// 优先使用下拉框选中的房间加入
	if (SelectedSessionIndex >= 0 && SessionManager)
	{
		SessionManager->JoinSession(SelectedSessionIndex, Name);
		return;
	}

	// Fallback: 通过 IP 地址直连
	FString IPAddress = IPAddressInput ? IPAddressInput->GetText().ToString() : TEXT("");
	if (IPAddress.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("[MainMenu] No room selected and no IP address provided"));
		return;
	}

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

void UPP_MainMenuWidget::OnRefreshButtonClicked()
{
	if (!SessionManager)
	{
		UE_LOG(LogTemp, Error, TEXT("[MainMenu] SessionManager is null!"));
		return;
	}

	// 显示搜索中提示
	if (RoomListDropdown)
	{
		RoomListDropdown->ClearOptions();
		RoomListDropdown->AddOption(TEXT("\u6b63\u5728\u641c\u7d22..."));
		RoomListDropdown->SetSelectedOption(TEXT("\u6b63\u5728\u641c\u7d22..."));
	}

	SelectedSessionIndex = -1;
	SessionManager->FindSessions();
}

void UPP_MainMenuWidget::OnSessionsFound(const TArray<FRoomInfo>& Rooms)
{
	UE_LOG(LogTemp, Log, TEXT("[MainMenu] Sessions found: %d"), Rooms.Num());

	if (!RoomListDropdown) return;

	RoomListDropdown->ClearOptions();
	SelectedSessionIndex = -1;

	if (Rooms.Num() == 0)
	{
		RoomListDropdown->AddOption(TEXT("\u6682\u65e0\u53ef\u7528\u623f\u95f4"));
		RoomListDropdown->SetSelectedOption(TEXT("\u6682\u65e0\u53ef\u7528\u623f\u95f4"));
		return;
	}

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
		// 刷新房间列表以获取最新状态
		if (RoomListDropdown)
		{
			RoomListDropdown->ClearOptions();
			RoomListDropdown->AddOption(TEXT("\u52a0\u5165\u5931\u8d25\uff0c\u8bf7\u91cd\u65b0\u5237\u65b0"));
			RoomListDropdown->SetSelectedOption(TEXT("\u52a0\u5165\u5931\u8d25\uff0c\u8bf7\u91cd\u65b0\u5237\u65b0"));
		}
		SelectedSessionIndex = -1;
	}
}