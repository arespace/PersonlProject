// Chen Tianbao uses GAS to create online battle games


#include "Player/PP_PlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Characters/PP_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameSession.h"
#include "GameplayTags/PPTags.h"
#include "UI/PP_HUD.h"
#include "UI/PP_UIViewModel.h"
#include "Game/PP_LobbyGameMode.h"
#include "Game/PP_SessionManager.h"
#include "Player/PP_LobbyPlayerState.h"
#include "UI/Lobby/PP_LobbyHUD.h"
#include "Dialogue/Actor/PP_NPCActor.h"
#include "Dialogue/Data/PP_DialogueData.h"
#include "Gather/Actor/PP_GatherableActor.h"
#include "UI/Dialogue/PP_DialogueWidget.h"
#include "UI/Shop/PP_ShopWidget.h"
#include "Shop/Component/PP_ShopComponent.h"
#include "Inventory/Component/PP_InventoryComponent.h"
#include "UI/PP_UIManager.h"
#include "Engine/DataTable.h"
#include "Quest/Component/PP_QuestComponent.h"
#include "Quest/Bridge/PP_QuestEventBridge.h"
#include "Game/PP_CrashGameMode.h"
#include "Player/PP_PlayerState.h"

void APP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;
	
	for (UInputMappingContext* Context : InputMappingContexts)
	{
		InputSubsystem->AddMappingContext(Context,0);
	}
	
	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;
	
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&ThisClass::Jump);
	EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Completed,this,&ThisClass::StopJumping);
	EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ThisClass::Move);
	EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&ThisClass::Look);
	
	EnhancedInputComponent->BindAction(PrimaryAction,ETriggerEvent::Started,this,&ThisClass::Primary);
	EnhancedInputComponent->BindAction(SecondaryAction,ETriggerEvent::Started,this,&ThisClass::Secondary);
	EnhancedInputComponent->BindAction(TertiaryAction,ETriggerEvent::Started,this,&ThisClass::Tertiary);
	EnhancedInputComponent->BindAction(FourthAction, ETriggerEvent::Started, this, &ThisClass::Fourth);
	EnhancedInputComponent->BindAction(FifthAction, ETriggerEvent::Started, this, &ThisClass::Fifth);
	
	// UI输入绑定
	if (PauseAction)
	{
		EnhancedInputComponent->BindAction(PauseAction, ETriggerEvent::Started, this, &ThisClass::TogglePauseMenu);
	}
	if (ScoreboardAction)
	{
		EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Started, this, &ThisClass::ShowScoreboard);
		EnhancedInputComponent->BindAction(ScoreboardAction, ETriggerEvent::Completed, this, &ThisClass::HideScoreboard);
	}
	if (HUDToggleAction)
	{
		EnhancedInputComponent->BindAction(HUDToggleAction, ETriggerEvent::Started, this, &ThisClass::ToggleHUD);
	}
	if (DialogueAction)
	{
		EnhancedInputComponent->BindAction(DialogueAction, ETriggerEvent::Started, this, &ThisClass::Interact);
	}
	if (QuestAction)
	{
		EnhancedInputComponent->BindAction(QuestAction, ETriggerEvent::Started, this, &ThisClass::ToggleQuestLog);
	}
	if (InventoryAction)
	{
		EnhancedInputComponent->BindAction(InventoryAction, ETriggerEvent::Started, this, &ThisClass::ToggleInventory);
	}
}

void APP_PlayerController::Jump()
{
	if (!IsValid(GetCharacter())) return;
	if (!IsAlive()) return;
	
	GetCharacter()->Jump();
}

void APP_PlayerController::StopJumping()
{
	if (!IsValid(GetCharacter())) return;
	if (!IsAlive()) return;
	
	GetCharacter()->StopJumping();
}

void APP_PlayerController::Move(const FInputActionValue& Value)
{
	if (!IsValid(GetPawn())) return;
	if (!IsAlive()) return;

	const FVector2D MoveMentVector = Value.Get<FVector2D>();
	
	//Find which way is forward
	const FRotator YawRotation(0.f, GetControlRotation().Yaw,0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	GetPawn()->AddMovementInput(ForwardDirection,MoveMentVector.Y);
	GetPawn()->AddMovementInput(RightDirection,MoveMentVector.X);
	
}

void APP_PlayerController::Look(const FInputActionValue& Value)
{
	if (!IsAlive()) return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();
	
	AddYawInput(LookAxisVector.X);
	AddPitchInput(LookAxisVector.Y);
	
}

void APP_PlayerController::Primary()
{
	ActivateAbility(PPTags::PPAbilities::Primary);
}

void APP_PlayerController::Secondary()
{
	ActivateAbility(PPTags::PPAbilities::Secondary);
}

void APP_PlayerController::Tertiary()
{
	ActivateAbility(PPTags::PPAbilities::Tertiary);
}


void APP_PlayerController::Fourth()
{
	ActivateAbility(PPTags::PPAbilities::Fourth);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("已执行第四个技能"));
}

void APP_PlayerController::Fifth()
{
	ActivateAbility(PPTags::PPAbilities::Fifth);
	GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, TEXT("已执行第五个技能"));
}

void APP_PlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{
	if (!IsAlive()) return;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());
	if (!IsValid(ASC)) return;
	
	ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());
}

bool APP_PlayerController::IsAlive() const
{
	APP_BaseCharacter* BaseCharacter = Cast<APP_BaseCharacter>(GetPawn());
	if (!IsValid(BaseCharacter)) return false;
	return BaseCharacter->IsAlive();
}

void APP_PlayerController::TogglePauseMenu()
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (!HUD)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] TogglePauseMenu - HUD is NULL"));
		return;
	}

	// 暂停菜单应始终可用，无需检查存活状态
	if (HUD->IsPauseMenuVisible())
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] TogglePauseMenu - Hiding pause menu"));
		HUD->HidePauseMenu();
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] TogglePauseMenu - Showing pause menu"));
		HUD->ShowPauseMenu();
	}
}

void APP_PlayerController::ShowScoreboard()
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->ShowScoreboard();
	}
}

void APP_PlayerController::HideScoreboard()
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->HideScoreboard();
	}
}

void APP_PlayerController::ToggleHUD()
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->ToggleMainHUD();
	}
}

void APP_PlayerController::Server_SendChatMessage_Implementation(const FString& Message)
{
	// 获取发送者名称（从 PlayerState）
	FString SenderName = TEXT("Unknown");
	if (PlayerState)
	{
		SenderName = PlayerState->GetPlayerName();
	}

	// 广播给所有玩家
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_PlayerController* PC = Cast<APP_PlayerController>(It->Get());
		if (IsValid(PC))
		{
			PC->Client_ReceiveChatMessage(SenderName, Message);
		}
	}

	// 通知任务系统：聊天消息发送事件
	APP_CrashGameMode* GM = GetWorld()->GetAuthGameMode<APP_CrashGameMode>();
	if (GM)
	{
		UPP_QuestEventBridge* Bridge = GM->GetQuestEventBridge();
		if (Bridge)
		{
			Bridge->NotifyChatSent(this);
		}
	}
}

void APP_PlayerController::Client_ReceiveChatMessage_Implementation(const FString& PlayerName, const FString& Message)
{
	// 尝试路由到大厅HUD
	APP_LobbyHUD* LobbyHUD = GetLobbyHUD();
	if (LobbyHUD)
	{
		LobbyHUD->AddChatMessage(PlayerName, Message);
		return;
	}

	// 回退到战斗HUD
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->AddNotification(FString::Printf(TEXT("[%s]: %s"), *PlayerName, *Message));
	}
}

void APP_PlayerController::Client_ReceiveNotification_Implementation(const FString& Message)
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->AddNotification(Message);
	}
}

void APP_PlayerController::Client_UpdateScoreboard_Implementation(const TArray<FScoreboardEntry>& Entries)
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		UPP_ScoreboardWidget* Scoreboard = HUD->GetScoreboardWidget();
		if (Scoreboard)
		{
			Scoreboard->UpdateScoreboard(Entries);
		}
	}
}

// ===== 大厅阶段 RPC 实现 =====

void APP_PlayerController::Server_SetPlayerName_Implementation(const FString& NewName)
{
	if (PlayerState && !NewName.IsEmpty())
	{
		PlayerState->SetPlayerName(NewName);
	}
}

void APP_PlayerController::Server_SetReady_Implementation(bool bReady)
{
	APP_LobbyPlayerState* LobbyPS = GetPlayerState<APP_LobbyPlayerState>();
	if (LobbyPS)
	{
		LobbyPS->Server_SetReady(bReady);
	}
}

void APP_PlayerController::Server_SelectCharacter_Implementation(FName CharacterId)
{
	APP_LobbyPlayerState* LobbyPS = GetPlayerState<APP_LobbyPlayerState>();
	if (LobbyPS)
	{
		LobbyPS->Server_SelectCharacter(CharacterId);
	}
}

void APP_PlayerController::Server_UpdateRoomSettings_Implementation(const FRoomSettings& Settings)
{
	// 仅主机可以更新房间设置
	APP_LobbyPlayerState* LobbyPS = GetPlayerState<APP_LobbyPlayerState>();
	if (!LobbyPS || !LobbyPS->IsHost()) return;

	APP_LobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<APP_LobbyGameMode>();
	if (LobbyGM)
	{
		LobbyGM->UpdateRoomSettings(Settings);
	}
}

void APP_PlayerController::Server_StartGame_Implementation()
{
	// 仅主机可以开始游戏
	APP_LobbyPlayerState* LobbyPS = GetPlayerState<APP_LobbyPlayerState>();
	if (!LobbyPS || !LobbyPS->IsHost()) return;

	APP_LobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<APP_LobbyGameMode>();
	if (LobbyGM)
	{
		LobbyGM->StartGame();
	}
}

void APP_PlayerController::Server_KickPlayer_Implementation(int32 PlayerId)
{
	APP_LobbyPlayerState* HostPS = GetPlayerState<APP_LobbyPlayerState>();
	if (!HostPS || !HostPS->IsHost()) return;

	APP_LobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<APP_LobbyGameMode>();
	if (!LobbyGM || !LobbyGM->GameSession) return;

	// 遍历找到对应序号的玩家并踢出
	for (FConstPlayerControllerIterator It = GetWorld()->GetPlayerControllerIterator(); It; ++It)
	{
		APP_LobbyPlayerState* LobbyPS = It->Get()->GetPlayerState<APP_LobbyPlayerState>();
		if (LobbyPS && LobbyPS->GetPlayerIndex() == PlayerId)
		{
			LobbyGM->GameSession->KickPlayer(It->Get(), FText::FromString(TEXT("Kicked by host")));
			break;
		}
	}
}

void APP_PlayerController::Client_OnLobbyPlayerListUpdated_Implementation(const TArray<FLobbyPlayerInfo>& Players)
{
	// World销毁期间收到RPC属正常情况，直接跳过
	if (!GetWorld() || GetWorld()->bIsTearingDown)
	{
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OnLobbyPlayerListUpdated - Players: %d"), Players.Num());

	APP_LobbyHUD* LobbyHUD = GetLobbyHUD();
	if (LobbyHUD)
	{
		LobbyHUD->OnPlayerListUpdated(Players);
	}
	else
	{
		UE_LOG(LogTemp, Verbose, TEXT("[PlayerController] Client_OnLobbyPlayerListUpdated - LobbyHUD is NULL"));
	}
}

void APP_PlayerController::Client_OnLobbySettingsUpdated_Implementation(const FRoomSettings& Settings)
{
	APP_LobbyHUD* LobbyHUD = GetLobbyHUD();
	if (LobbyHUD)
	{
		LobbyHUD->OnRoomSettingsUpdated(Settings);
	}
}

void APP_PlayerController::Client_OnGameStartingCountdown_Implementation(float Countdown)
{
	APP_LobbyHUD* LobbyHUD = GetLobbyHUD();
	if (LobbyHUD)
	{
		LobbyHUD->OnGameStarting(Countdown);
	}
}

APP_LobbyHUD* APP_PlayerController::GetLobbyHUD() const
{
	if (!IsLocalController()) return nullptr;
	return Cast<APP_LobbyHUD>(GetHUD());
}

// ===== 房主迁移 RPC 实现 =====

void APP_PlayerController::Server_RequestHostMigration_Implementation()
{
	// 验证调用者是房主
	APP_LobbyPlayerState* LobbyPS = GetPlayerState<APP_LobbyPlayerState>();
	if (!LobbyPS || !LobbyPS->IsHost())
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server_RequestHostMigration - caller is not host, ignoring"));
		return;
	}

	APP_LobbyGameMode* LobbyGM = GetWorld()->GetAuthGameMode<APP_LobbyGameMode>();
	if (LobbyGM)
	{
		LobbyGM->HandleHostMigration(this);
	}
}

void APP_PlayerController::Client_PrepareHostMigration_Implementation(const FHostMigrationData& MigrationData)
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_PrepareHostMigration - becoming new host"));

	UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
	if (SM)
	{
		SM->SetMigrationData(MigrationData);
	}
}

void APP_PlayerController::Client_NotifyHostMigrating_Implementation(const FHostMigrationData& MigrationData)
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_NotifyHostMigrating - host is migrating to: %s"), *MigrationData.NewHostPlayerName);

	UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
	if (SM)
	{
		SM->SetMigrationData(MigrationData);
	}
}

// ===== NPC对话系统 RPC 实现 =====

void APP_PlayerController::Server_RequestStartDialogue_Implementation(APP_NPCActor* NPC)
{
	if (!IsValid(NPC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server_RequestStartDialogue - NPC is null"));
		return;
	}

	// 距离验证：玩家 Pawn 与 NPC 间的水平距离不得超过 InteractMaxDistance
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server_RequestStartDialogue - No controlled pawn"));
		return;
	}

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), NPC->GetActorLocation());
	if (Distance > InteractMaxDistance)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server_RequestStartDialogue - Too far: %.1f > %.1f"),
			Distance, InteractMaxDistance);
		return;
	}

	// 调用 NPC 开始对话（服务端权威）
	NPC->StartDialogue(this);
}

void APP_PlayerController::Client_OpenDialogueUI_Implementation(APP_NPCActor* NPC, FName DialogueID)
{
	if (!IsLocalController()) return;

	if (!IsValid(NPC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenDialogueUI - NPC is null"));
		return;
	}

	// 从 NPC 的对话表中查找行数据
	UDataTable* DT = NPC->GetDialogueTable();
	if (!IsValid(DT))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenDialogueUI - NPC DialogueTable is null"));
		return;
	}

	FPP_DialogueData* Row = DT->FindRow<FPP_DialogueData>(DialogueID, TEXT("Client_OpenDialogueUI"));
	if (!Row)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenDialogueUI - DialogueID '%s' not found in table"), *DialogueID.ToString());
		return;
	}

	// 确保 DialogueID 字段被正确填充（DataTable 行键可能已设置，但成员变量可能为空）
	FPP_DialogueData DataCopy = *Row;
	if (DataCopy.DialogueID.IsNone())
	{
		DataCopy.DialogueID = DialogueID;
	}

	// 创建对话 Widget
	if (!DialogueWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenDialogueUI - DialogueWidgetClass not configured"));
		return;
	}

	UPP_DialogueWidget* DialogueWidget = CreateWidget<UPP_DialogueWidget>(this, DialogueWidgetClass);
	if (!DialogueWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerController] Client_OpenDialogueUI - Failed to create DialogueWidget"));
		return;
	}

	DialogueWidget->AddToViewport(90);
	DialogueWidget->SetupDialogue(DataCopy, NPC, this);

	UE_LOG(LogTemp, Log, TEXT("[PlayerController] Client_OpenDialogueUI - Opened dialogue '%s' with NPC '%s'"),
		*DialogueID.ToString(), *NPC->GetName());
}

void APP_PlayerController::Server_ConfirmDialogueEndAction_Implementation(APP_NPCActor* NPC, FName DialogueID, EPP_DialogueEndAction Action)
{
	if (!IsValid(NPC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - NPC is null"));
		return;
	}

	// ★ 先更新 TalkToNPC 目标进度，再处理 SubmitQuest/OfferQuest 等动作
	//    否则 SubmitQuest 检查目标完成时 TalkToNPC 还是 0，导致需要提交两次
	if (!NPC->NPCID.IsNone())
	{
		APP_CrashGameMode* GM = GetWorld()->GetAuthGameMode<APP_CrashGameMode>();
		if (GM)
		{
			UPP_QuestEventBridge* Bridge = GM->GetQuestEventBridge();
			if (Bridge)
			{
				Bridge->NotifyDialogueCompleted(this, NPC->NPCID);
			}
		}
	}

	switch (Action)
	{
	case EPP_DialogueEndAction::OpenShop:
	{
		// 服务端确认后通知客户端打开商店 UI
		UPP_ShopComponent* Shop = NPC->GetShopComponent();
		if (!IsValid(Shop))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - NPC has no ShopComponent"));
			return;
		}
		Client_OpenShopUI(NPC);
		break;
	}
	case EPP_DialogueEndAction::OfferQuest:
	{
		// 接入任务系统——调用 QuestComponent->AcceptQuest(RelatedQuestID)
		UDataTable* DT = NPC->GetDialogueTable();
		if (IsValid(DT))
		{
			FPP_DialogueData* Row = DT->FindRow<FPP_DialogueData>(DialogueID, TEXT("OfferQuest"));
			if (Row && !Row->RelatedQuestID.IsNone())
			{
				APP_PlayerState* PS = GetPlayerState<APP_PlayerState>();
				if (PS)
				{
					UPP_QuestComponent* QC = PS->GetQuestComponent();
					if (QC)
					{
						QC->AcceptQuest(Row->RelatedQuestID);
					}
				}
			}
		}
		// 广播对话结束事件
		OnDialogueEnded.Broadcast(DialogueID, FName());
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - OfferQuest for DialogueID '%s'"), *DialogueID.ToString());
		break;
	}
	case EPP_DialogueEndAction::SubmitQuest:
	{
		// 接入任务系统——调用 QuestComponent->SubmitQuest(RelatedQuestID)
		UDataTable* SubmitDT = NPC->GetDialogueTable();
		if (IsValid(SubmitDT))
		{
			FPP_DialogueData* SubmitRow = SubmitDT->FindRow<FPP_DialogueData>(DialogueID, TEXT("SubmitQuest"));
			if (SubmitRow && !SubmitRow->RelatedQuestID.IsNone())
			{
				APP_PlayerState* PS = GetPlayerState<APP_PlayerState>();
				if (PS)
				{
					UPP_QuestComponent* QC = PS->GetQuestComponent();
					if (QC)
					{
						QC->SubmitQuest(SubmitRow->RelatedQuestID);
					}
				}
			}
		}
		// 广播对话结束事件
		OnDialogueEnded.Broadcast(DialogueID, FName());
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - SubmitQuest for DialogueID '%s'"), *DialogueID.ToString());
		break;
	}
	case EPP_DialogueEndAction::ChainNextDialogue:
	{
		// 查找当前对话的 NextDialogueID，重新触发 Client_OpenDialogueUI
		UDataTable* DT = NPC->GetDialogueTable();
		if (!IsValid(DT))
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerController] ChainNextDialogue - NPC DialogueTable is null"));
			return;
		}

		FPP_DialogueData* Row = DT->FindRow<FPP_DialogueData>(DialogueID, TEXT("ChainNextDialogue"));
		if (!Row || Row->NextDialogueID.IsNone())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerController] ChainNextDialogue - no NextDialogueID for '%s'"), *DialogueID.ToString());
			return;
		}

		Client_OpenDialogueUI(NPC, Row->NextDialogueID);
		break;
	}
	case EPP_DialogueEndAction::None:
	default:
		UE_LOG(LogTemp, Log, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - Action=None, dialogue ended."));
		break;
	}

	// 通知任务系统：与 NPC 对话完成（TalkToNPC 任务目标）
	if (!NPC->NPCID.IsNone())
	{
		APP_CrashGameMode* GM = GetWorld()->GetAuthGameMode<APP_CrashGameMode>();
		if (GM)
		{
			UPP_QuestEventBridge* Bridge = GM->GetQuestEventBridge();
			if (Bridge)
			{
				Bridge->NotifyDialogueCompleted(this, NPC->NPCID);
			}
		}
	}
}

void APP_PlayerController::Client_OpenShopUI_Implementation(APP_NPCActor* NPC)
{
	if (!IsLocalController()) return;

	if (!IsValid(NPC))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenShopUI - NPC is null"));
		return;
	}

	UPP_ShopComponent* Shop = NPC->GetShopComponent();
	if (!IsValid(Shop))
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenShopUI - NPC ShopComponent is null"));
		return;
	}

	if (!ShopWidgetClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_OpenShopUI - ShopWidgetClass not configured"));
		return;
	}

	UPP_ShopWidget* ShopWidget = CreateWidget<UPP_ShopWidget>(this, ShopWidgetClass);
	if (!ShopWidget)
	{
		UE_LOG(LogTemp, Error, TEXT("[PlayerController] Client_OpenShopUI - Failed to create ShopWidget"));
		return;
	}

	// 获取玩家的 InventoryComponent（假定挂在 PlayerState 上）
	UPP_InventoryComponent* Inventory = nullptr;
	if (PlayerState)
	{
		Inventory = PlayerState->FindComponentByClass<UPP_InventoryComponent>();
	}

	ShopWidget->AddToViewport(static_cast<int32>(EPP_UILayer::Overlay));
	ShopWidget->InitializeShop(Shop, Inventory);

	UE_LOG(LogTemp, Log, TEXT("[PlayerController] Client_OpenShopUI - Opened shop for NPC '%s'"), *NPC->GetName());
}

void APP_PlayerController::Interact()
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact called"));
	if (!IsLocalController()) return;

	// 从相机前方做线性 trace 寻找 NPC
	APlayerCameraManager* CamMgr = PlayerCameraManager;
	if (!CamMgr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact - No CameraManager"));
		return;
	}

	FVector Start = CamMgr->GetCameraLocation();
	FVector End = Start + CamMgr->GetCameraRotation().Vector() * InteractTraceDistance;

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetPawn());

	bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params);
	if (!bHit)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact - No hit on Visibility channel"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact - Hit: %s"), *Hit.GetActor()->GetName());

	APP_NPCActor* NPC = Cast<APP_NPCActor>(Hit.GetActor());
	if (NPC)
	{
		// 调用 NPC 的 Interact（内部会判断服务器/客户端并转发）
		NPC->Interact(this);
		return;
	}

	// 采集物交互：通过 Server RPC 走完整的采集流程
	APP_GatherableActor* Gatherable = Cast<APP_GatherableActor>(Hit.GetActor());
	if (Gatherable)
	{
		if (!Gatherable->IsAvailable())
		{
			UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact - Gatherable NOT available: %s"), *Gatherable->GetName());
			return;
		}
		UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact - Gatherable HIT: %s, ItemID=%s"), *Gatherable->GetName(), *Gatherable->ItemID.ToString());
		Server_InteractGather(Gatherable);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Interact - Unknown actor class=%s name=%s"), *Hit.GetActor()->GetClass()->GetName(), *Hit.GetActor()->GetName());
}

void APP_PlayerController::Server_InteractGather_Implementation(APP_GatherableActor* Target)
{
	UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] ENTER: Target=%s"), *GetNameSafe(Target));

	if (!IsValid(Target) || !Target->IsAvailable())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] FAIL - Target unavailable or null"));
		return;
	}

	APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] FAIL - No pawn"));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] Calling Server_StartGather: Target=%s ItemID=%s GatherTime=%.1f"),
		*Target->GetName(), *Target->ItemID.ToString(), Target->GetGatherTime());

	Target->Server_StartGather(PlayerPawn);

	// 用计时器模拟采集过程，完成后触发 CompleteGather
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel = FTimerDelegate::CreateWeakLambda(this, [this, Target, PlayerPawn]()
	{
		UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] TIMER FIRED: Target=%s ItemID=%s"),
			*GetNameSafe(Target), Target ? *Target->ItemID.ToString() : TEXT("null"));

		if (IsValid(Target) && IsValid(PlayerPawn))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] Calling CompleteGather..."));
			Target->CompleteGather(PlayerPawn);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] TIMER FAIL - Target=%s Pawn=%s"),
				*GetNameSafe(Target), *GetNameSafe(PlayerPawn));
		}
	});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, Target->GetGatherTime(), false);

	UE_LOG(LogTemp, Warning, TEXT("[Server_InteractGather] Timer set for %.1fs"), Target->GetGatherTime());
}

void APP_PlayerController::ToggleQuestLog()
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->ToggleQuestLog();
	}
}

void APP_PlayerController::ToggleInventory()
{
	APP_HUD* HUD = Cast<APP_HUD>(GetHUD());
	if (HUD)
	{
		HUD->ToggleInventory();
	}
}
