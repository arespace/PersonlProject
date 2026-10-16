// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_LobbyGameMode.h"
#include "Framework/PP_SessionManager.h"
#include "Player/PP_LobbyPlayerState.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "GameFramework/SpectatorPawn.h"
#include "GameFramework/HUD.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "Framework/PP_Log.h"

APP_LobbyGameMode::APP_LobbyGameMode()
{
	// 大厅使用旁观者Pawn
	DefaultPawnClass = ASpectatorPawn::StaticClass();
	// 允许无缝切换
	bUseSeamlessTravel = true;

	// 【重要】构造函数中不做任何蓝图同步加载（包括 ConstructorHelpers::FClassFinder）。
	// 构造函数在 CDO 构造期执行，而 CDO 构造发生在主玩法模块（PersonalProject）加载时，
	// 此时 UI 模块（PersonalProjectUI）尚未注册；若在此加载引用了 UI 类的蓝图
	// （如 BP_PP_PlayerController 默认引用了 WBP_DialogueBox / WBP_Shop，
	//   而这两个 WBP 的父类 PP_DialogueWidget / PP_ShopWidget 属于 UI 模块），
	// 会导致 /Script/PersonalProjectUI 脚本包找不到，触发
	// 「VerifyImport: Failed to find script package」+「Failed to load Class ... as Parent」
	// 及其级联的 WidgetTree 加载失败。因此构造函数仅设置 C++ 兜底类，
	// 蓝图类改由 InitGame 在运行时（所有模块加载完成后）加载。
	PlayerStateClass = APP_LobbyPlayerState::StaticClass();
	PlayerControllerClass = APP_PlayerController::StaticClass();

	// HUDClass 设计上由蓝图（BP_LobbyGameMode）配置——主玩法模块不能 include UI 头文件
	// （模块化拆分：Game 不依赖 UI），故 C++ 不硬编码 APP_LobbyHUD。
	// 但蓝图若未配置 HUDClass，大厅会生成默认 AHUD，APP_LobbyHUD 不会生成，
	// 导致大厅/主菜单界面为空。因此改由 InitGame 运行时兜底（仅在未配置时生效）。
}

void APP_LobbyGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
	Super::InitGame(MapName, Options, ErrorMessage);

	// 运行时按路径加载蓝图类：此时所有模块（含 UI）均已注册，蓝图引用可正常解析。
	// 这里刻意使用字符串路径而非 CDO 默认值，避免把 UI 资产拖进模块加载期的同步加载。
	if (UClass* LobbyPCClass = LoadClass<APlayerController>(nullptr, TEXT("/Game/PersonalProject/Player/BP_PP_PlayerController.BP_PP_PlayerController_C")))
	{
		PlayerControllerClass = LobbyPCClass;
	}
	if (UClass* LobbyPSClass = LoadClass<APlayerState>(nullptr, TEXT("/Game/PersonalProject/UI/LobbyInterface/Components/BP_LobbyPlayerState.BP_LobbyPlayerState_C")))
	{
		PlayerStateClass = LobbyPSClass;
	}

	// HUD 兜底：蓝图（BP_LobbyGameMode）未配置 HUDClass 时，基类默认是 AHUD::StaticClass()，
	// 大厅会生成默认 AHUD，APP_LobbyHUD 不会生成，导致主菜单/大厅界面为空。
	// 这里仅在仍是默认 AHUD（或空）时，运行时加载 BP_LobbyHUD 兜底。
	if (HUDClass == nullptr || HUDClass == AHUD::StaticClass())
	{
		if (UClass* LobbyHUDClass = LoadClass<AHUD>(nullptr, TEXT("/Game/PersonalProject/UI/LobbyInterface/Core/BP_LobbyHUD.BP_LobbyHUD_C")))
		{
			HUDClass = LobbyHUDClass;
		}
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
				UE_LOG(LogPP, Verbose, TEXT("[LobbyGameMode] Logout - host transferred to: %s"), *CandidatePS->GetPlayerName());
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

	// 将每个玩家的角色选择存入 SessionManager（跨 SeamlessTravel 持久化）
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
UE_LOG(LogPP, Verbose, TEXT("[LobbyGameMode] StartGame - ServerTravel to: %s"), *TravelURL);
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
	UE_LOG(LogPP, Verbose, TEXT("[LobbyGameMode] HandleHostMigration - host is leaving"));

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
		UE_LOG(LogPP, Verbose, TEXT("[LobbyGameMode] HandleHostMigration - no other players, just disconnecting"));
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
	UE_LOG(LogPP, Verbose, TEXT("[LobbyGameMode] HandleHostMigration - new host: %s"), *NewHostName);

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
	UE_LOG(LogPP, Verbose, TEXT("[LobbyGameMode] ExecuteHostDisconnect - shutting down server"));

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