// Chen Tianbao uses GAS to create online battle games


#include "Player/PP_PlayerController.h"
#include "Player/PP_UIGateway.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Characters/PP_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/HUD.h"
#include "GameplayTags/PPTags.h"
#include "Game/PP_LobbyGameMode.h"
#include "Game/PP_SessionManager.h"
#include "Player/PP_LobbyPlayerState.h"
#include "Dialogue/PP_NPCActor.h"
#include "Data/PP_DialogueData.h"
#include "Gather/PP_GatherableActor.h"
#include "Interaction/PP_InteractableInterface.h"
#include "Shop/PP_ShopComponent.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Engine/DataTable.h"
#include "Quest/PP_QuestComponent.h"
#include "Quest/PP_QuestEventBridge.h"
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

// ===== UI 网关转发（实现体在 UI 模块的 APP_HUD / APP_LobbyHUD，通过 IPP_UIGateway 接口调用） =====

void APP_PlayerController::TogglePauseMenu()
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestTogglePauseMenu(GetHUD());
	}
}

void APP_PlayerController::ShowScoreboard()
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestShowScoreboard(GetHUD());
	}
}

void APP_PlayerController::HideScoreboard()
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestHideScoreboard(GetHUD());
	}
}

void APP_PlayerController::ToggleHUD()
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestToggleMainHUD(GetHUD());
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
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_PushChatMessage(GetHUD(), PlayerName, Message);
	}
}

void APP_PlayerController::Client_ReceiveNotification_Implementation(const FString& Message)
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_PushNotification(GetHUD(), Message);
	}
}

void APP_PlayerController::Client_UpdateScoreboard_Implementation(const TArray<FScoreboardEntry>& Entries)
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestUpdateScoreboard(GetHUD(), Entries);
	}
}

void APP_PlayerController::Client_NotifyQuestMessage_Implementation(const FString& Message)
{
	if (!IsLocalController()) return;

	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_PushNotification(GetHUD(), Message);
	}
}

// ===== IPP_DialogueRequester 接口实现（NPC 通过接口请求对话，转发到自身 RPC） =====

void APP_PlayerController::RequestStartDialogue_Implementation(APP_NPCActor* NPC)
{
	Server_RequestStartDialogue(NPC);
}

void APP_PlayerController::OpenDialogueUI_Implementation(APP_NPCActor* NPC, FName DialogueID)
{
	Client_OpenDialogueUI(NPC, DialogueID);
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
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_HandlePlayerListUpdated(GetHUD(), Players);
	}
}

void APP_PlayerController::Client_OnLobbySettingsUpdated_Implementation(const FRoomSettings& Settings)
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_HandleRoomSettingsUpdated(GetHUD(), Settings);
	}
}

void APP_PlayerController::Client_OnGameStartingCountdown_Implementation(float Countdown)
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_HandleGameStarting(GetHUD(), Countdown);
	}
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

// Client_NotifyHostMigrating：大厅提示条推送
void APP_PlayerController::Client_NotifyHostMigrating_Implementation(const FHostMigrationData& MigrationData)
{
	UE_LOG(LogTemp, Warning, TEXT("[PlayerController] Client_NotifyHostMigrating - host is migrating to: %s"), *MigrationData.NewHostPlayerName);

	UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
	if (SM)
	{
		SM->SetMigrationData(MigrationData);
	}

	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_HandleHostMigrating(GetHUD(), MigrationData);
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

// Client_OpenDialogueUI / Client_OpenShopUI：通过 UI 网关接口转发（实现体在 UI 模块）
void APP_PlayerController::Client_OpenDialogueUI_Implementation(APP_NPCActor* NPC, FName DialogueID)
{
	if (!IsLocalController()) return;

	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestOpenDialogue(GetHUD(), NPC, DialogueID);
	}
}

void APP_PlayerController::Client_OpenShopUI_Implementation(APP_NPCActor* NPC)
{
	if (!IsLocalController()) return;

	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestOpenShop(GetHUD(), NPC);
	}
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
						// 提交失败时通知客户端（目标未完成/任务未接），避免静默无反馈
						if (!QC->SubmitQuest(SubmitRow->RelatedQuestID))
						{
							Client_NotifyQuestMessage(FString::Printf(
								TEXT("任务「%s」的目标尚未全部完成，无法提交"),
								*SubmitRow->RelatedQuestID.ToString()));
						}
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

	// 通过交互接口统一处理，无需关心具体类型（NPC/采集物/宝箱等）
	if (IPP_Interactable* Interactable = Cast<IPP_Interactable>(Hit.GetActor()))
	{
		if (Interactable->CanInteract(this))
		{
			Interactable->Interact(this);
		}
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
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestToggleQuestLog(GetHUD());
	}
}

void APP_PlayerController::ToggleInventory()
{
	if (IPP_UIGateway* GW = Cast<IPP_UIGateway>(GetHUD()))
	{
		IPP_UIGateway::Execute_RequestToggleInventory(GetHUD());
	}
}
