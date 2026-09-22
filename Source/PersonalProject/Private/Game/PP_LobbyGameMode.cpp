// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_LobbyGameMode.h"
#include "Game/PP_GameInstance.h"
#include "Game/PP_SessionManager.h"
#include "Player/PP_LobbyPlayerState.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "GameFramework/SpectatorPawn.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "UObject/ConstructorHelpers.h"

APP_LobbyGameMode::APP_LobbyGameMode()
{
	// 大厅使用旁观者Pawn
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	// 绑定BP_LobbyPlayerState蓝图到GameMode
	static ConstructorHelpers::FClassFinder<APlayerState> LobbyPSClassFinder(TEXT("/Game/PersonalProject/UI/LobbyInterface/Components/BP_LobbyPlayerState.BP_LobbyPlayerState_C"));
	if (LobbyPSClassFinder.Succeeded())
	{
		PlayerStateClass = LobbyPSClassFinder.Class;
	}
	else
	{
		PlayerStateClass = APP_LobbyPlayerState::StaticClass();
	}
	// 允许无缝切换
	bUseSeamlessTravel = true;

	// 绑定BP_LobbyHUD蓝图到GameMode
	static ConstructorHelpers::FClassFinder<AHUD> LobbyHUDClassFinder(TEXT("/Game/PersonalProject/UI/LobbyInterface/Core/BP_LobbyHUD.BP_LobbyHUD_C"));
	if (LobbyHUDClassFinder.Succeeded())
	{
		HUDClass = LobbyHUDClassFinder.Class;
	}
	else
	{
		HUDClass = APP_LobbyHUD::StaticClass();
	}

	// 绑定BP_PP_PlayerController蓝图到GameMode
	static ConstructorHelpers::FClassFinder<APlayerController> LobbyPCClassFinder(TEXT("/Game/PersonalProject/Player/BP_PP_PlayerController.BP_PP_PlayerController_C"));
	if (LobbyPCClassFinder.Succeeded())
	{
		PlayerControllerClass = LobbyPCClassFinder.Class;
	}
	else
	{
		PlayerControllerClass = APP_PlayerController::StaticClass();
	}
}

void APP_LobbyGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	// 检查是否已有房主
	bool bHasHost = false;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get() == NewPlayer) continue;
		APP_LobbyPlayerState* ExistingPS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
		if (ExistingPS && ExistingPS->IsHost())
		{
			bHasHost = true;
			break;
		}
	}
	if (!bHasHost)
	{
		APP_LobbyPlayerState* NewPS = NewPlayer->GetPlayerState<APP_LobbyPlayerState>();
		if (NewPS)
		{
			NewPS->SetIsHost(true);
		}
	}

	AssignPlayerIndices();

	// 延迟广播，等待客户端 HUD 初始化完成
	FTimerHandle BroadcastTimerHandle;
	GetWorldTimerManager().SetTimer(BroadcastTimerHandle, this, &APP_LobbyGameMode::BroadcastPlayerListUpdate, 0.5f, false);
}

FString APP_LobbyGameMode::InitNewPlayer(APlayerController* NewPlayerController, const FUniqueNetIdRepl& UniqueId, const FString& Options, const FString& Portal)
{
	FString ErrorMessage = Super::InitNewPlayer(NewPlayerController, UniqueId, Options, Portal);

	// 从连接 Options 解析玩家名并设置到 PlayerState
	FString PlayerName = UGameplayStatics::ParseOption(Options, TEXT("PlayerName"));
	if (PlayerName.IsEmpty())
	{
		// 编辑器模式或未设置名字时，使用默认名字
		int32 PlayerCount = GetNumPlayers();
		PlayerName = FString::Printf(TEXT("Player_%d"), PlayerCount);
	}

	if (NewPlayerController && NewPlayerController->PlayerState)
	{
		NewPlayerController->PlayerState->SetPlayerName(PlayerName);
	}

	return ErrorMessage;
}

void APP_LobbyGameMode::Logout(AController* Exiting)
{
	// 检查离开的玩家是否是房主（处理非迁移场景的异常断开）
	APP_LobbyPlayerState* ExitingPS = Exiting ? Exiting->GetPlayerState<APP_LobbyPlayerState>() : nullptr;
	bool bWasHost = ExitingPS && ExitingPS->IsHost();

	Super::Logout(Exiting);

	// PIE退出或引擎关闭时，跳过房主迁移避免无效广播
	if (UWorld* W = GetWorld())
	{
		if (W->bIsTearingDown || IsEngineExitRequested())
		{
			return;
		}
	}
	else
	{
		return;
	}

	// 如果离开的是房主，转移房主给下一个玩家
	if (bWasHost)
	{
		for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
		{
			APP_LobbyPlayerState* CandidatePS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
			if (CandidatePS && !CandidatePS->IsHost())
			{
				CandidatePS->SetIsHost(true);
				UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] Logout - host transferred to: %s"), *CandidatePS->GetPlayerName());
				break;
			}
		}
	}

	AssignPlayerIndices();
	BroadcastPlayerListUpdate();
}

bool APP_LobbyGameMode::CheckAllPlayersReady() const
{
	// 至少需要1个玩家
	if (GetWorld()->GetNumControllers() < 1)
	{
		return false;
	}

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_LobbyPlayerState* LobbyPS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
		if (LobbyPS && !LobbyPS->IsReady())
		{
			return false;
		}
	}

	return true;
}

void APP_LobbyGameMode::StartGame()
{
	if (!CheckAllPlayersReady())
	{
		return;
	}

	// 将每个玩家的角色选择存入（跨 SeamlessTravel 持久化）
	// 同时存入 GameInstance（真正持久）和 SessionManager（备用）
	UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld());
	UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_PlayerController* PC = Cast<APP_PlayerController>(It->Get());
		if (PC)
		{
			APP_LobbyPlayerState* LobbyPS = PC->GetPlayerState<APP_LobbyPlayerState>();
			if (LobbyPS)
			{
				const FString& PlayerName = LobbyPS->GetPlayerName();
				const FName CharId = LobbyPS->GetSelectedCharacterId();


				// 直接存到 PlayerController 上（最可靠，PC 跨地图不销毁）
			PC->SetPendingCharacterId(CharId);

			if (GI)
			{
				GI->SetPlayerCharacterSelection(PlayerName, CharId);
			}
			if (SM)
			{
				SM->SetPlayerCharacterSelection(PlayerName, CharId);
			}
		}
		PC->Client_OnGameStartingCountdown(3.f);
		}
	}

	// 延迟旅行，让倒计时动画播放
	FTimerHandle TravelTimerHandle;
	GetWorldTimerManager().SetTimer(TravelTimerHandle, [this]()
	{
		FString TravelURL = FString::Printf(TEXT("/Game/PersonalProject/Maps/%s?Listen"), *RoomSettings.MapName);
UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] StartGame - ServerTravel to: %s"), *TravelURL);
		GetWorld()->ServerTravel(TravelURL);
	}, 3.f, false);
}

void APP_LobbyGameMode::UpdateRoomSettings(const FRoomSettings& NewSettings)
{
	RoomSettings = NewSettings;
	BroadcastPlayerListUpdate();
}

void APP_LobbyGameMode::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_LobbyGameMode, RoomSettings);
}

TArray<FLobbyPlayerInfo> APP_LobbyGameMode::GetLobbyPlayerList() const
{
	TArray<FLobbyPlayerInfo> PlayerList;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_LobbyPlayerState* LobbyPS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
		if (LobbyPS)
		{
			FLobbyPlayerInfo Info;
			Info.PlayerName = LobbyPS->GetPlayerName();
			Info.bIsReady = LobbyPS->IsReady();
			Info.SelectedCharacterId = LobbyPS->GetSelectedCharacterId();
			Info.PlayerIndex = LobbyPS->GetPlayerIndex();
			Info.bIsHost = LobbyPS->IsHost();
			PlayerList.Add(Info);
		}
	}

	return PlayerList;
}

void APP_LobbyGameMode::BroadcastPlayerListUpdate()
{
	// World销毁期间不向客户端发送RPC
	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		return;
	}

	TArray<FLobbyPlayerInfo> PlayerList = GetLobbyPlayerList();

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_PlayerController* PC = Cast<APP_PlayerController>(It->Get());
		if (IsValid(PC))
		{
			PC->Client_OnLobbyPlayerListUpdated(PlayerList);
			PC->Client_OnLobbySettingsUpdated(RoomSettings);
		}
	}
}

void APP_LobbyGameMode::AssignPlayerIndices()
{
	int32 Index = 0;
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_LobbyPlayerState* LobbyPS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
		if (LobbyPS)
		{
			LobbyPS->SetPlayerIndex(Index++);
		}
	}
}

void APP_LobbyGameMode::HandleHostMigration(AController* LeavingHost)
{
	UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] HandleHostMigration - host is leaving"));

	// 找到下一个非离开的玩家作为新房主
	APP_PlayerController* NewHostPC = nullptr;
	APP_LobbyPlayerState* NewHostPS = nullptr;

	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get() == LeavingHost) continue;

		APP_LobbyPlayerState* CandidatePS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
		if (CandidatePS)
		{
			NewHostPC = Cast<APP_PlayerController>(It->Get());
			NewHostPS = CandidatePS;
			break;
		}
	}

	if (!NewHostPC || !NewHostPS)
	{
		UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] HandleHostMigration - no other players, just disconnecting"));
		// 没有其他玩家，直接断开
		APlayerController* HostPC = Cast<APlayerController>(LeavingHost);
		if (HostPC)
		{
			HostPC->ConsoleCommand(TEXT("Disconnect"));
		}
		return;
	}

	// 收集当前房间状态
	FString NewHostName = NewHostPS->GetPlayerName();

	// 准备新房主的迁移数据
	FHostMigrationData NewHostData;
	NewHostData.bPendingMigration = true;
	NewHostData.bIsNewHost = true;
	NewHostData.PlayerName = NewHostName;
	NewHostData.NewHostPlayerName = NewHostName;
	NewHostData.SavedRoomSettings = RoomSettings;
	NewHostData.MaxPlayers = RoomSettings.MaxPlayers;

	// 向新房主发送迁移数据
	NewHostPC->Client_PrepareHostMigration(NewHostData);
	UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] HandleHostMigration - new host: %s"), *NewHostName);

	// 向其他客户端发送迁移通知
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		if (It->Get() == LeavingHost || It->Get() == NewHostPC) continue;

		APP_PlayerController* OtherPC = Cast<APP_PlayerController>(It->Get());
		if (OtherPC)
		{
			// 获取该客户端的玩家名
			APP_LobbyPlayerState* OtherPS = OtherPC->GetPlayerState<APP_LobbyPlayerState>();
			FString OtherPlayerName = OtherPS ? OtherPS->GetPlayerName() : TEXT("Player");

			FHostMigrationData ClientData;
			ClientData.bPendingMigration = true;
			ClientData.bIsNewHost = false;
			ClientData.PlayerName = OtherPlayerName;
			ClientData.NewHostPlayerName = NewHostName;
			ClientData.SavedRoomSettings = RoomSettings;
			ClientData.MaxPlayers = RoomSettings.MaxPlayers;

			OtherPC->Client_NotifyHostMigrating(ClientData);
		}
	}

	// 延迟 1 秒断开，确保 RPC 送达
	GetWorldTimerManager().SetTimer(HostDisconnectTimer, this, &APP_LobbyGameMode::ExecuteHostDisconnect, 1.0f, false);
}

void APP_LobbyGameMode::ExecuteHostDisconnect()
{
	UE_LOG(LogTemp, Warning, TEXT("[LobbyGameMode] ExecuteHostDisconnect - shutting down server"));

	APlayerController* HostPC = GetWorld()->GetFirstPlayerController();
	if (HostPC)
	{
		HostPC->ConsoleCommand(TEXT("Disconnect"));
	}
}

void APP_LobbyGameMode::OnRep_RoomSettings()
{
	// 客户端房间设置变更回调，可通过HUD更新UI
}