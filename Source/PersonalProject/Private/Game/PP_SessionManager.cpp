// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_SessionManager.h"
#include "OnlineSubsystem.h"
#include "OnlineSessionSettings.h"
#include "Online/OnlineSessionNames.h" // from OnlineBase module
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void UPP_SessionManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Subsystem Initialized - lifetime follows GameInstance"));

	// 验证 SessionInterface 可用性
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Log, TEXT("[SessionManager] Online Session Interface available"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Online Session Interface NOT available at initialization!"));
	}
}

UPP_SessionManager* UPP_SessionManager::Get(const UWorld* World)
{
	if (!World) return nullptr;
	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UPP_SessionManager>() : nullptr;
}

void UPP_SessionManager::Deinitialize()
{
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Subsystem Deinitializing"));

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
		if (ExistingSession)
		{
			// 检查当前是否为 Host（ListenServer 或 DedicatedServer）
			UWorld* World = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
			bool bIsHost = World && (World->GetNetMode() == NM_ListenServer || World->GetNetMode() == NM_DedicatedServer);

			if (!bIsHost)
			{
				// Client 断开时清理本地 Session 状态
				SessionInterface->DestroySession(NAME_GameSession);
				UE_LOG(LogTemp, Log, TEXT("[SessionManager] Client session destroyed during Deinitialize"));
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Host session preserved during Deinitialize"));
			}
		}
	}

	Super::Deinitialize();
}

IOnlineSessionPtr UPP_SessionManager::GetSessionInterface() const
{
	IOnlineSubsystem* OnlineSub = IOnlineSubsystem::Get();
	if (OnlineSub)
	{
		return OnlineSub->GetSessionInterface();
	}
	return nullptr;
}

void UPP_SessionManager::CreateSession(const FString& PlayerName, int32 MaxPlayers)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionManager] Online Subsystem Session interface not available!"));
		OnCreateSessionComplete.Broadcast(false);
		return;
	}

	PendingPlayerName = PlayerName;

	// 先销毁已有 Session
	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		SessionInterface->DestroySession(NAME_GameSession);
	}

	// 配置 Session 设置
	FOnlineSessionSettings SessionSettings;
	SessionSettings.bIsLANMatch = true;
	SessionSettings.NumPublicConnections = MaxPlayers;
	SessionSettings.bShouldAdvertise = true;
	SessionSettings.bUsesPresence = false;
	SessionSettings.bAllowJoinInProgress = true;
	SessionSettings.bAllowJoinViaPresence = false;
	SessionSettings.Set(FName("SERVER_NAME"), PlayerName, EOnlineDataAdvertisementType::ViaOnlineServiceAndPing);

	// 绑定回调
	CreateSessionDelegateHandle = SessionInterface->AddOnCreateSessionCompleteDelegate_Handle(
		FOnCreateSessionCompleteDelegate::CreateUObject(this, &UPP_SessionManager::OnCreateSessionCompleteInternal));

	SessionInterface->CreateSession(0, NAME_GameSession, SessionSettings);
}

void UPP_SessionManager::OnCreateSessionCompleteInternal(FName SessionName, bool bSuccess)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnCreateSessionCompleteDelegate_Handle(CreateSessionDelegateHandle);
	}

	UE_LOG(LogTemp, Warning, TEXT("[SessionManager] CreateSession %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
	OnCreateSessionComplete.Broadcast(bSuccess);
}

void UPP_SessionManager::FindSessions()
{
#if WITH_EDITOR
	if (GetGameInstance() && GetGameInstance()->GetWorld() && GetGameInstance()->GetWorld()->IsPlayInEditor())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Running in PIE mode - LAN session discovery requires separate processes. Set 'Run Under One Process = false' in Play settings."));
	}
#endif

	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionManager] Session interface not available for FindSessions!"));
		return;
	}

	SearchSettings = MakeShareable(new FOnlineSessionSearch());
	SearchSettings->bIsLanQuery = true;
	SearchSettings->MaxSearchResults = 20;
	// LAN search does not need SEARCH_PRESENCE query setting

	FindSessionsDelegateHandle = SessionInterface->AddOnFindSessionsCompleteDelegate_Handle(
		FOnFindSessionsCompleteDelegate::CreateUObject(this, &UPP_SessionManager::OnFindSessionsCompleteInternal));

	SessionInterface->FindSessions(0, SearchSettings.ToSharedRef());
}

void UPP_SessionManager::OnFindSessionsCompleteInternal(bool bSuccess)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnFindSessionsCompleteDelegate_Handle(FindSessionsDelegateHandle);
	}

	LastSearchResults.Empty();

	if (bSuccess && SearchSettings.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] FindSessions found %d results"), SearchSettings->SearchResults.Num());

		for (int32 i = 0; i < SearchSettings->SearchResults.Num(); i++)
		{
			const FOnlineSessionSearchResult& Result = SearchSettings->SearchResults[i];
			FRoomInfo Info;

			FString ServerName;
			if (Result.Session.SessionSettings.Get(FName("SERVER_NAME"), ServerName))
			{
				Info.RoomName = ServerName;
			}
			else
			{
				Info.RoomName = FString::Printf(TEXT("Room_%d"), i);
			}

			Info.MaxPlayers = Result.Session.SessionSettings.NumPublicConnections;
			Info.CurrentPlayers = Info.MaxPlayers - Result.Session.NumOpenPublicConnections;
			Info.Ping = Result.PingInMs;

			LastSearchResults.Add(Info);
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] FindSessions failed or no results"));
	}

	OnSessionsFound.Broadcast(LastSearchResults);
}

void UPP_SessionManager::JoinSession(int32 SessionIndex, const FString& PlayerName)
{
	JoinPlayerName = PlayerName;
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid() || !SearchSettings.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionManager] Cannot JoinSession - invalid state"));
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	if (!SearchSettings->SearchResults.IsValidIndex(SessionIndex))
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionManager] Invalid session index: %d"), SessionIndex);
		OnJoinSessionComplete.Broadcast(false);
		return;
	}

	JoinSessionDelegateHandle = SessionInterface->AddOnJoinSessionCompleteDelegate_Handle(
		FOnJoinSessionCompleteDelegate::CreateUObject(this, &UPP_SessionManager::OnJoinSessionCompleteInternal));

	SessionInterface->JoinSession(0, NAME_GameSession, SearchSettings->SearchResults[SessionIndex]);
}

void UPP_SessionManager::OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnJoinSessionCompleteDelegate_Handle(JoinSessionDelegateHandle);
	}

	bool bSuccess = (Result == EOnJoinSessionCompleteResult::Success);
	UE_LOG(LogTemp, Warning, TEXT("[SessionManager] JoinSession %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));

	if (bSuccess && SessionInterface.IsValid())
	{
		// 获取连接字符串并执行 ClientTravel
		FString ConnectString;
		if (SessionInterface->GetResolvedConnectString(NAME_GameSession, ConnectString))
		{
			// 追加 PlayerName 到 Travel URL
			if (!JoinPlayerName.IsEmpty())
			{
				ConnectString += FString::Printf(TEXT("?PlayerName=%s"), *JoinPlayerName);
			}
			UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Traveling to: %s"), *ConnectString);

			UWorld* World = GetWorld();
			if (!World)
			{
				// UObject::GetWorld() 可能返回nullptr，通过 GEngine 获取
				if (GEngine)
				{
					World = GEngine->GetWorldContexts().Num() > 0 ? GEngine->GetWorldContexts()[0].World() : nullptr;
				}
			}

			if (World)
			{
				APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
				if (PC)
				{
					PC->ClientTravel(ConnectString, TRAVEL_Absolute);
				}
			}
			else
			{
				UE_LOG(LogTemp, Error, TEXT("[SessionManager] Cannot get World for ClientTravel!"));
			}
		}
	}

	OnJoinSessionComplete.Broadcast(bSuccess);
}

void UPP_SessionManager::DestroyCurrentSession()
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (!SessionInterface.IsValid()) return;

	auto ExistingSession = SessionInterface->GetNamedSession(NAME_GameSession);
	if (ExistingSession)
	{
		DestroySessionDelegateHandle = SessionInterface->AddOnDestroySessionCompleteDelegate_Handle(
			FOnDestroySessionCompleteDelegate::CreateUObject(this, &UPP_SessionManager::OnDestroySessionCompleteInternal));
		SessionInterface->DestroySession(NAME_GameSession);
	}
}

void UPP_SessionManager::OnDestroySessionCompleteInternal(FName SessionName, bool bSuccess)
{
	IOnlineSessionPtr SessionInterface = GetSessionInterface();
	if (SessionInterface.IsValid())
	{
		SessionInterface->ClearOnDestroySessionCompleteDelegate_Handle(DestroySessionDelegateHandle);
	}
	UE_LOG(LogTemp, Warning, TEXT("[SessionManager] DestroySession %s"), bSuccess ? TEXT("SUCCESS") : TEXT("FAILED"));
	OnDestroySessionComplete.Broadcast(bSuccess);
}

// ===== 房主迁移实现 =====

void UPP_SessionManager::SetMigrationData(const FHostMigrationData& Data)
{
	PendingMigration = Data;
	UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Migration data set - IsNewHost: %s, PlayerName: %s, NewHostName: %s"),
		PendingMigration.bIsNewHost ? TEXT("true") : TEXT("false"),
		*PendingMigration.PlayerName,
		*PendingMigration.NewHostPlayerName);
}

void UPP_SessionManager::ClearMigrationData()
{
	PendingMigration = FHostMigrationData();
	MigrationPollCount = 0;

	// 清理轮询定时器
	UWorld* World = GetWorld();
	if (!World)
	{
		if (GetGameInstance() && GetGameInstance()->GetWorld())
		{
			World = GetGameInstance()->GetWorld();
		}
	}
	if (World)
	{
		World->GetTimerManager().ClearTimer(MigrationPollTimer);
	}

	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Migration data cleared"));
}

void UPP_SessionManager::ExecuteHostMigration_AsNewHost()
{
	UE_LOG(LogTemp, Warning, TEXT("[SessionManager] ExecuteHostMigration_AsNewHost - creating session as: %s"), *PendingMigration.PlayerName);

	// 构造 Travel URL（Session 创建成功后执行）
	MigrationTravelURL = FString::Printf(TEXT("Open LobbyMap?Listen?PlayerName=%s"), *PendingMigration.PlayerName);

	// 绑定一次性回调
	OnCreateSessionComplete.AddDynamic(this, &UPP_SessionManager::OnMigrationSessionCreated);

	// 创建 Session
	CreateSession(PendingMigration.PlayerName, PendingMigration.MaxPlayers);
}

void UPP_SessionManager::OnMigrationSessionCreated(bool bSuccess)
{
	// 移除回调（一次性）
	OnCreateSessionComplete.RemoveDynamic(this, &UPP_SessionManager::OnMigrationSessionCreated);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Migration session created, traveling: %s"), *MigrationTravelURL);

		ClearMigrationData();

		// 执行 ServerTravel
		UWorld* World = GetWorld();
		if (!World && GetGameInstance())
		{
			World = GetGameInstance()->GetWorld();
		}
		if (World)
		{
			APlayerController* PC = World->GetFirstPlayerController();
			if (PC)
			{
				PC->ConsoleCommand(MigrationTravelURL);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[SessionManager] Migration session creation FAILED!"));
		ClearMigrationData();
		// 失败时不做额外处理，玩家会看到主菜单
	}
}

void UPP_SessionManager::ExecuteHostMigration_AsClient()
{
	UE_LOG(LogTemp, Warning, TEXT("[SessionManager] ExecuteHostMigration_AsClient - searching for host: %s"), *PendingMigration.NewHostPlayerName);

	MigrationPollCount = 0;

	// 绑定搜索结果回调
	OnSessionsFound.AddDynamic(this, &UPP_SessionManager::OnMigrationSessionsFound);

	// 使用定时器轮询，初次延迟 2 秒（给新房主时间创建 Session）
	UWorld* World = GetWorld();
	if (!World && GetGameInstance())
	{
		World = GetGameInstance()->GetWorld();
	}
	if (World)
	{
		World->GetTimerManager().SetTimer(MigrationPollTimer, [this]()
		{
			MigrationPollCount++;
			UE_LOG(LogTemp, Log, TEXT("[SessionManager] Migration poll %d/%d"), MigrationPollCount, MaxMigrationPolls);

			if (MigrationPollCount > MaxMigrationPolls)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Migration poll timeout, giving up"));
				OnSessionsFound.RemoveDynamic(this, &UPP_SessionManager::OnMigrationSessionsFound);
				ClearMigrationData();
				return;
			}

			FindSessions();
		}, 2.0f, true, 3.0f); // 首次延迟 3 秒，之后每 2 秒
	}
}

void UPP_SessionManager::OnMigrationSessionsFound(const TArray<FRoomInfo>& Rooms)
{
	if (!HasPendingMigration()) return;

	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Migration search found %d rooms, looking for: %s"), Rooms.Num(), *PendingMigration.NewHostPlayerName);

	// 在搜索结果中查找匹配新房主名称的 Session
	for (int32 i = 0; i < Rooms.Num(); i++)
	{
		if (Rooms[i].RoomName == PendingMigration.NewHostPlayerName)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionManager] Found migration target session at index %d: %s"), i, *Rooms[i].RoomName);

			// 停止轮询
			UWorld* World = GetWorld();
			if (!World && GetGameInstance())
			{
				World = GetGameInstance()->GetWorld();
			}
			if (World)
			{
				World->GetTimerManager().ClearTimer(MigrationPollTimer);
			}

			// 移除回调
			OnSessionsFound.RemoveDynamic(this, &UPP_SessionManager::OnMigrationSessionsFound);

			// 保存玩家名，然后清除迁移数据
			FString PlayerName = PendingMigration.PlayerName;
			ClearMigrationData();

			// 加入 Session
			JoinSession(i, PlayerName);
			return;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[SessionManager] Migration target not found yet, will retry..."));
}

// ===== 角色选择桥接 =====

void UPP_SessionManager::SetPlayerCharacterSelection(const FString& PlayerName, FName CharacterId)
{
	if (PlayerName.IsEmpty() || CharacterId.IsNone()) return;

	PlayerCharacterSelections.Add(PlayerName, CharacterId);
	LastCharacterId = CharacterId;  // 后备方案：不依赖 PlayerName 匹配
	UE_LOG(LogTemp, Log, TEXT("[SessionManager] SetPlayerCharacterSelection - Player: %s, Character: %s"),
		*PlayerName, *CharacterId.ToString());
}

FName UPP_SessionManager::GetPlayerCharacterSelection(const FString& PlayerName) const
{
	if (const FName* Found = PlayerCharacterSelections.Find(PlayerName))
	{
		return *Found;
	}
	return NAME_None;
}

FName UPP_SessionManager::ConsumePlayerCharacterSelection(const FString& PlayerName)
{
	FName CharacterId = NAME_None;
	if (const FName* Found = PlayerCharacterSelections.Find(PlayerName))
	{
		CharacterId = *Found;
		PlayerCharacterSelections.Remove(PlayerName);
		UE_LOG(LogTemp, Log, TEXT("[SessionManager] ConsumePlayerCharacterSelection - Player: %s, Character: %s"),
			*PlayerName, *CharacterId.ToString());
	}
	return CharacterId;
}

void UPP_SessionManager::ClearPlayerCharacterSelection(const FString& PlayerName)
{
	PlayerCharacterSelections.Remove(PlayerName);
}

void UPP_SessionManager::ClearAllPlayerCharacterSelections()
{
	PlayerCharacterSelections.Empty();
	LastCharacterId = NAME_None;
}
