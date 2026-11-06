

#include "Player/PP_PlayerController.h"
#include "Player/PP_UIGateway.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "EnhancedInputSubsystems.h"
#include "EnhancedInputComponent.h"
#include "Characters/PP_BaseCharacter.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameSession.h"
#include "GameFramework/HUD.h"
#include "GameplayTags/PPTags.h"
#include "Game/PP_LobbyGameMode.h"
#include "Session/PP_SessionManager.h"
#include "Player/PP_LobbyPlayerState.h"
#include "Dialogue/PP_NPCActor.h"
#include "Data/PP_DialogueData.h"
#include "Gather/PP_GatherableActor.h"
#include "GameObjects/PP_BonfireActor.h"
#include "Interfaces/PP_InteractableInterface.h"
#include "Shop/PP_ShopComponent.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Engine/DataTable.h"
#include "Quest/PP_QuestComponent.h"
#include "Quest/PP_QuestEventBridge.h"
#include "Game/PP_CrashGameMode.h"
#include "Player/PP_PlayerState.h"
#include "Framework/PP_Log.h"

void APP_PlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	UEnhancedInputLocalPlayerSubsystem* InputSubsystem =ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer());
	if (!IsValid(InputSubsystem)) return;
	
	// PC 侧旧 IMC 恢复——ALS IMC（priority=0）处理视角/移动，PC 侧旧 IMC（priority=1）处理技能/UI。
	// 两者按键不冲突（ALS 用鼠标XY/WASD，PC侧用鼠标1-5键/功能键），priority 设 1 保证 ALS 鼠标路由优先。
	for (UInputMappingContext* Context : InputMappingContexts)
	{
		InputSubsystem->AddMappingContext(Context, 1);
	}

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!IsValid(EnhancedInputComponent)) return;
	
	// 移动与跳跃统一交给 ALS：IMC_Als_Default → IA_Als_Move / IA_Als_Jump
	// → APP_PlayerCharacter::Input_OnMove / Input_OnJump。
	// IMC_Movement 中的 WASD→IA_Move、Space→IA_Jump 映射已删除，避免 PC 侧抢占导致
	// ALS 的步态/蹲伏/翻滚等逻辑收不到输入。
	// EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Started,this,&ThisClass::Jump);
	// EnhancedInputComponent->BindAction(JumpAction,ETriggerEvent::Completed,this,&ThisClass::StopJumping);
	// EnhancedInputComponent->BindAction(MoveAction,ETriggerEvent::Triggered,this,&ThisClass::Move);
	// 鼠标视角统一交给 ALS：IMC_Als_Default(priority=0) → IA_Als_LookMouse
	// → APP_PlayerCharacter::Input_OnLookMouse。
	// IMC_Movement 中的 Mouse2D → IA_Look 映射已删除，避免它与 ALS 争抢鼠标 XY。
	// EnhancedInputComponent->BindAction(LookAction,ETriggerEvent::Triggered,this,&ThisClass::Look);
	
	EnhancedInputComponent->BindAction(PrimaryAction,ETriggerEvent::Started,this,&ThisClass::Primary);
	EnhancedInputComponent->BindAction(SecondaryAction,ETriggerEvent::Started,this,&ThisClass::Secondary);
	EnhancedInputComponent->BindAction(TertiaryAction,ETriggerEvent::Started,this,&ThisClass::Tertiary);
	EnhancedInputComponent->BindAction(FourthAction, ETriggerEvent::Started, this, &ThisClass::Fourth);
	EnhancedInputComponent->BindAction(FifthAction, ETriggerEvent::Started, this, &ThisClass::Fifth);

	// 排错：IA 资产若为空，BindAction 会静默失败，技能按键完全不会进 ActivateAbility
	UE_LOG(LogPP, Log, TEXT("[PP_Skill] 技能按键绑定: 主=%s 次=%s 三=%s 四=%s 五=%s"),
		*GetNameSafe(PrimaryAction), *GetNameSafe(SecondaryAction), *GetNameSafe(TertiaryAction),
		*GetNameSafe(FourthAction), *GetNameSafe(FifthAction));
	
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

	const FVector2D MoveVector = Value.Get<FVector2D>();
	
	//Find which way is forward
	const FRotator YawRotation(0.f, GetControlRotation().Yaw,0.f);
	const FVector ForwardDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
	const FVector RightDirection = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);
	
	GetPawn()->AddMovementInput(ForwardDirection,MoveVector.Y);
	GetPawn()->AddMovementInput(RightDirection,MoveVector.X);
	
}

void APP_PlayerController::Look(const FInputActionValue& Value)
{
	if (!IsAlive()) return;
	const FVector2D LookAxisVector = Value.Get<FVector2D>();

	AddYawInput(LookAxisVector.X);
	// IMC_Movement 的 Mouse2D → IA_Look 映射自带 Swizzle + Negate 修饰器，
	// 传进来的 Y 与实际鼠标上下方向相反，这里取负抵消，保证"鼠标上移=抬头"。
	AddPitchInput(-LookAxisVector.Y);
	
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
}

void APP_PlayerController::Fifth()
{
	ActivateAbility(PPTags::PPAbilities::Fifth);
}

void APP_PlayerController::ActivateAbility(const FGameplayTag& AbilityTag) const
{
	APP_BaseCharacter* BaseCharacter = Cast<APP_BaseCharacter>(GetPawn());
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetPawn());

	FGameplayTagContainer OwnedTags;
	if (IsValid(ASC))
	{
		ASC->GetOwnedGameplayTags(OwnedTags);
	}

	const UPP_AttributeSet* AttributeSet = IsValid(ASC) ? ASC->GetSet<UPP_AttributeSet>() : nullptr;

	// 排错：技能按键入口把拦截条件（存活/Dead 标签/ASC/控制权/体力/激活抑制）全部输出
	UE_LOG(LogPP, Warning,
		TEXT("[PP_Skill] 按键: 能力=%s 存活=%d bAlive=%d DeadTag=%d 本地控制=%d LocalRole=%d 体力=%.1f/%.1f 抑制=%d Pawn=%s ASC=%s 拥有标签=[%s]"),
		*AbilityTag.ToString(), IsAlive() ? 1 : 0,
		BaseCharacter ? (BaseCharacter->IsAlive() ? 1 : 0) : -1,
		(IsValid(ASC) && ASC->HasMatchingGameplayTag(PPTags::Status::Dead)) ? 1 : 0,
		IsLocalController() ? 1 : 0, static_cast<int32>(GetLocalRole()),
		AttributeSet ? AttributeSet->GetStamina() : -1.f,
		AttributeSet ? AttributeSet->GetMaxStamina() : -1.f,
		(IsValid(ASC) && ASC->GetUserAbilityActivationInhibited()) ? 1 : 0,
		*GetNameSafe(GetPawn()), *GetNameSafe(ASC), *OwnedTags.ToStringSimple());

	if (!IsAlive())
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 未能释放：IsAlive 为 false（角色=%s）"), *GetNameSafe(BaseCharacter));
		return;
	}
	if (!IsValid(ASC))
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 未能释放：ASC 无效（Pawn=%s）"), *GetNameSafe(GetPawn()));
		return;
	}

	// 排错：无论成功与否都打印匹配到的能力数量。大于 1 说明同一技能被重复授予，
	// 一次按键会激活多份实例、多扣体力。
	TArray<FGameplayAbilitySpec*> MatchingSpecs;
	ASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		AbilityTag.GetSingleTagContainer(), MatchingSpecs, false);
	UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 匹配能力数=%d"), MatchingSpecs.Num());

	const bool bActivated = ASC->TryActivateAbilitiesByTag(AbilityTag.GetSingleTagContainer());

	if (!bActivated)
	{
		// 排错：列出所有匹配该标签的能力，并给出 CanActivateAbility 的失败原因标签
		for (FGameplayAbilitySpec* Spec : MatchingSpecs)
		{
			FGameplayTagContainer FailureTags;
			const bool bCanActivate = Spec && Spec->Ability && ASC->AbilityActorInfo.IsValid()
				&& Spec->Ability->CanActivateAbility(
					Spec->Handle, ASC->AbilityActorInfo.Get(), nullptr, nullptr, &FailureTags);

			UE_LOG(LogPP, Warning,
				TEXT("[PP_Skill]   能力=%s CanActivate=%d 失败原因=[%s] 激活中=%d 实例=%s"),
				*GetNameSafe(Spec ? Spec->Ability : nullptr), bCanActivate ? 1 : 0,
				*FailureTags.ToStringSimple(), Spec ? Spec->ActiveCount : -1,
				*GetNameSafe(Spec ? Spec->GetPrimaryInstance() : nullptr));
		}
	}

	UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 激活结果: %s → %s 体力后=%.1f/%.1f"),
		*AbilityTag.ToString(), bActivated ? TEXT("成功") : TEXT("失败"),
		AttributeSet ? AttributeSet->GetStamina() : -1.f,
		AttributeSet ? AttributeSet->GetMaxStamina() : -1.f);
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
	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestTogglePauseMenu);
}

void APP_PlayerController::ShowScoreboard()
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestShowScoreboard);
}

void APP_PlayerController::HideScoreboard()
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestHideScoreboard);
}

void APP_PlayerController::ToggleHUD()
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestToggleMainHUD);
}

void APP_PlayerController::ServerInteractBonfire_Implementation(APP_BonfireActor* Bonfire)
{
	if (IsValid(Bonfire))
	{
		Bonfire->Rest(this);
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
	ForwardToUIGateway(&IPP_UIGateway::Execute_PushChatMessage, PlayerName, Message);
}

void APP_PlayerController::Client_NotifyQuestMessage_Implementation(const FString& Message)
{
	if (!IsLocalController()) return;

	ForwardToUIGateway(&IPP_UIGateway::Execute_PushNotification, Message);
}

void APP_PlayerController::Client_NotifyMessage_Implementation(const FString& Message)
{
	if (!IsLocalController()) return;

	ForwardToUIGateway(&IPP_UIGateway::Execute_PushNotification, Message);
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

// UI 侧（对话 Widget）经 Execute_ 调用：转发到 Server RPC
void APP_PlayerController::ConfirmDialogueEndAction_Implementation(UObject* NPC, FName DialogueID, EPP_DialogueEndAction Action)
{
	Server_ConfirmDialogueEndAction(Cast<APP_NPCActor>(NPC), DialogueID, Action);
}

// ===== IPP_LobbyActions 接口实现（UI 大厅 Widget 经 Execute_ 调用，转发到 Server RPC）=====

void APP_PlayerController::RequestSetReady_Implementation(bool bReady)
{
	Server_SetReady(bReady);
}

void APP_PlayerController::RequestStartGame_Implementation()
{
	Server_StartGame();
}

void APP_PlayerController::RequestHostMigration_Implementation()
{
	Server_RequestHostMigration();
}

void APP_PlayerController::RequestSendChatMessage_Implementation(const FString& Message)
{
	Server_SendChatMessage(Message);
}

void APP_PlayerController::RequestUpdateRoomSettings_Implementation(const FRoomSettings& Settings)
{
	Server_UpdateRoomSettings(Settings);
}

void APP_PlayerController::RequestSelectCharacter_Implementation(FName CharacterId)
{
	Server_SelectCharacter(CharacterId);
}

// ===== 大厅阶段 RPC 实现 =====

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

void APP_PlayerController::Client_OnLobbyPlayerListUpdated_Implementation(const TArray<FLobbyPlayerInfo>& Players)
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_HandlePlayerListUpdated, Players);
}

void APP_PlayerController::Client_OnLobbySettingsUpdated_Implementation(const FRoomSettings& Settings)
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_HandleRoomSettingsUpdated, Settings);
}

void APP_PlayerController::Client_OnGameStartingCountdown_Implementation(float Countdown)
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_HandleGameStarting, Countdown);
}

// ===== 房主迁移 RPC 实现 =====

void APP_PlayerController::Server_RequestHostMigration_Implementation()
{
	// 验证调用者是房主
	APP_LobbyPlayerState* LobbyPS = GetPlayerState<APP_LobbyPlayerState>();
	if (!LobbyPS || !LobbyPS->IsHost())
	{
		UE_LOG(LogPP, Warning, TEXT("[PlayerController] Server_RequestHostMigration - caller is not host, ignoring"));
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
	UE_LOG(LogPP, Verbose, TEXT("[PlayerController] Client_PrepareHostMigration - becoming new host"));

	UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
	if (SM)
	{
		SM->SetMigrationData(MigrationData);
	}
}

// Client_NotifyHostMigrating：大厅提示条推送
void APP_PlayerController::Client_NotifyHostMigrating_Implementation(const FHostMigrationData& MigrationData)
{
	UE_LOG(LogPP, Verbose, TEXT("[PlayerController] Client_NotifyHostMigrating - host is migrating to: %s"), *MigrationData.NewHostPlayerName);

	UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
	if (SM)
	{
		SM->SetMigrationData(MigrationData);
	}

	ForwardToUIGateway(&IPP_UIGateway::Execute_HandleHostMigrating, MigrationData);
}

// ===== NPC对话系统 RPC 实现 =====

void APP_PlayerController::Server_RequestStartDialogue_Implementation(APP_NPCActor* NPC)
{
	if (!IsValid(NPC))
	{
		UE_LOG(LogPP, Warning, TEXT("[PlayerController] Server_RequestStartDialogue - NPC is null"));
		return;
	}

	// 距离验证：玩家 Pawn 与 NPC 间的水平距离不得超过 InteractMaxDistance
	APawn* ControlledPawn = GetPawn();
	if (!IsValid(ControlledPawn))
	{
		UE_LOG(LogPP, Warning, TEXT("[PlayerController] Server_RequestStartDialogue - No controlled pawn"));
		return;
	}

	const float Distance = FVector::Dist(ControlledPawn->GetActorLocation(), NPC->GetActorLocation());
	if (Distance > InteractMaxDistance)
	{
		UE_LOG(LogPP, Warning, TEXT("[PlayerController] Server_RequestStartDialogue - Too far: %.1f > %.1f"),
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

	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestOpenDialogue, NPC, DialogueID, DialogueWidgetClass);
}

void APP_PlayerController::Client_OpenShopUI_Implementation(APP_NPCActor* NPC)
{
	if (!IsLocalController()) return;

	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestOpenShop, NPC, ShopWidgetClass);
}

void APP_PlayerController::Server_ConfirmDialogueEndAction_Implementation(APP_NPCActor* NPC, FName DialogueID, EPP_DialogueEndAction Action)
{
	if (!IsValid(NPC))
	{
		UE_LOG(LogPP, Warning, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - NPC is null"));
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

	// 统一查询本段对话行，OfferQuest / SubmitQuest / ChainNextDialogue 三个分支共用，
	// 避免各自重复「判空 + FindRow」
	FPP_DialogueData* Row = NPC->FindDialogueRow(DialogueID);

	// 预取玩家状态与任务组件，OfferQuest/SubmitQuest 分支共用，避免重复判空与查找
	APP_PlayerState* PS = GetPlayerState<APP_PlayerState>();
	UPP_QuestComponent* QC = PS ? PS->GetQuestComponent() : nullptr;

	switch (Action)
	{
	case EPP_DialogueEndAction::OpenShop:
	{
		// 服务端确认后通知客户端打开商店 UI
		UPP_ShopComponent* Shop = NPC->GetShopComponent();
		if (!IsValid(Shop))
		{
			UE_LOG(LogPP, Warning, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - NPC has no ShopComponent"));
			return;
		}
		Client_OpenShopUI(NPC);
		break;
	}
	case EPP_DialogueEndAction::OfferQuest:
	{
		// 接入任务系统——调用 QuestComponent->AcceptQuest(RelatedQuestID)
		if (Row && !Row->RelatedQuestID.IsNone() && QC)
		{
			QC->AcceptQuest(Row->RelatedQuestID);
		}
		UE_LOG(LogPP, Log, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - OfferQuest for DialogueID '%s'"), *DialogueID.ToString());
		break;
	}
	case EPP_DialogueEndAction::SubmitQuest:
	{
		// 接入任务系统——调用 QuestComponent->SubmitQuest(RelatedQuestID)
		if (Row && !Row->RelatedQuestID.IsNone() && QC)
		{
			// 提交失败时通知客户端（目标未完成/任务未接），避免静默无反馈
			if (!QC->SubmitQuest(Row->RelatedQuestID))
			{
				Client_NotifyQuestMessage(FString::Printf(
					TEXT("任务「%s」的目标尚未全部完成，无法提交"),
					*Row->RelatedQuestID.ToString()));
			}
		}
		UE_LOG(LogPP, Log, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - SubmitQuest for DialogueID '%s'"), *DialogueID.ToString());
		break;
	}
	case EPP_DialogueEndAction::ChainNextDialogue:
	{
		// 跳到当前对话的 NextDialogueID，重新触发 Client_OpenDialogueUI
		if (!Row || Row->NextDialogueID.IsNone())
		{
			UE_LOG(LogPP, Warning, TEXT("[PlayerController] ChainNextDialogue - no NextDialogueID for '%s'"), *DialogueID.ToString());
			return;
		}

		Client_OpenDialogueUI(NPC, Row->NextDialogueID);
		break;
	}
	case EPP_DialogueEndAction::None:
	default:
		UE_LOG(LogPP, Log, TEXT("[PlayerController] Server_ConfirmDialogueEndAction - Action=None, dialogue ended."));
		break;
	}
}

void APP_PlayerController::Interact()
{
	UE_LOG(LogPP, Log, TEXT("[PP_Interact] 按下交互键，开始相机射线检测"));
	if (!IsLocalController()) return;

	// 从相机前方做线性 trace 寻找可交互对象
	APlayerCameraManager* CamMgr = PlayerCameraManager;
	if (!CamMgr)
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Interact] 没有 PlayerCameraManager"));
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
		UE_LOG(LogPP, Warning, TEXT("[PP_Interact] 相机射线未命中任何物体（距离 %.0f cm）"), InteractTraceDistance);
		return;
	}

	UE_LOG(LogPP, Log, TEXT("[PP_Interact] 命中: %s（组件 %s）"),
		*GetNameSafe(Hit.GetActor()), *GetNameSafe(Hit.GetComponent()));

	// 通过交互接口统一处理，无需关心具体类型（NPC/采集物/篝火等）
	if (IPP_Interactable* Interactable = Cast<IPP_Interactable>(Hit.GetActor()))
	{
		if (Interactable->CanInteract(this))
		{
			Interactable->Interact(this);
		}
		else
		{
			UE_LOG(LogPP, Warning, TEXT("[PP_Interact] 命中的 %s 当前不可交互"), *GetNameSafe(Hit.GetActor()));
		}
		return;
	}

	UE_LOG(LogPP, Warning, TEXT("[PP_Interact] 命中的 Actor 没有实现交互接口: class=%s name=%s"),
		*Hit.GetActor()->GetClass()->GetName(), *Hit.GetActor()->GetName());
}

void APP_PlayerController::Server_InteractGather_Implementation(APP_GatherableActor* Target)
{
	UE_LOG(LogPP, Verbose, TEXT("[Server_InteractGather] ENTER: Target=%s"), *GetNameSafe(Target));

	if (!IsValid(Target) || !Target->IsAvailable())
	{
		UE_LOG(LogPP, Warning, TEXT("[Server_InteractGather] FAIL - Target unavailable or null"));
		return;
	}

	APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn)
	{
		UE_LOG(LogPP, Warning, TEXT("[Server_InteractGather] FAIL - No pawn"));
		return;
	}

	UE_LOG(LogPP, Verbose, TEXT("[Server_InteractGather] Calling Server_StartGather: Target=%s ItemID=%s GatherTime=%.1f"),
		*Target->GetName(), *Target->ItemID.ToString(), Target->GetGatherTime());

	Target->Server_StartGather(PlayerPawn);

	// 用计时器模拟采集过程，完成后触发 CompleteGather
	FTimerHandle TimerHandle;
	FTimerDelegate TimerDel = FTimerDelegate::CreateWeakLambda(this, [this, Target, PlayerPawn]()
	{
		UE_LOG(LogPP, Verbose, TEXT("[Server_InteractGather] TIMER FIRED: Target=%s ItemID=%s"),
			*GetNameSafe(Target), Target ? *Target->ItemID.ToString() : TEXT("null"));

		if (IsValid(Target) && IsValid(PlayerPawn))
		{
			UE_LOG(LogPP, Verbose, TEXT("[Server_InteractGather] Calling CompleteGather..."));
			Target->CompleteGather(PlayerPawn);
		}
		else
		{
			UE_LOG(LogPP, Warning, TEXT("[Server_InteractGather] TIMER FAIL - Target=%s Pawn=%s"),
				*GetNameSafe(Target), *GetNameSafe(PlayerPawn));
		}
	});

	GetWorld()->GetTimerManager().SetTimer(TimerHandle, TimerDel, Target->GetGatherTime(), false);

	UE_LOG(LogPP, Verbose, TEXT("[Server_InteractGather] Timer set for %.1fs"), Target->GetGatherTime());
}

void APP_PlayerController::ToggleQuestLog()
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestToggleQuestLog);
}

void APP_PlayerController::ToggleInventory()
{
	ForwardToUIGateway(&IPP_UIGateway::Execute_RequestToggleInventory);
}
