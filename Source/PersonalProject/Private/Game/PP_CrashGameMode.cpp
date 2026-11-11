
#include "Game/PP_CrashGameMode.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "Player/PP_PlayerController.h"
#include "Characters/PP_PlayerCharacter.h"
#include "Player/PP_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "Events/PP_EventBus.h"
#include "Events/PP_EventTypes.h"
#include "Game/Save/PP_SaveSubsystem.h"
#include "Quest/PP_QuestEventBridge.h"
#include "Game/PP_EnemyRespawnManager.h"
#include "Session/PP_SessionManager.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_Log.h"

APP_CrashGameMode::APP_CrashGameMode()
{
	// C++ 默认值仅设置玩法侧类；HUDClass 留空由蓝图（BP_CrashGameMode）配置，
	// 避免主玩法模块 include UI 头文件（模块化拆分：Game 不依赖 UI）
	PlayerControllerClass = APP_PlayerController::StaticClass();
	DefaultPawnClass = APP_PlayerCharacter::StaticClass();
	PlayerStateClass = APP_PlayerState::StaticClass();

	// 任务事件桥接器组件
	QuestEventBridge = CreateDefaultSubobject<UPP_QuestEventBridge>("QuestEventBridge");

	// 敌人重生管理器（篝火休息时按半径重置敌人）
	EnemyRespawnManager = CreateDefaultSubobject<UPP_EnemyRespawnManager>("EnemyRespawnManager");
}

void APP_CrashGameMode::BeginPlay()
{
	Super::BeginPlay();

	// Boss 死亡事件只用于本局记录（不额外广播），Boss Actor 被销毁后仍能保存"已击败"
	if (UPP_EventBus* Bus = UPP_EventBus::Get(this))
	{
		BossDeadHandle = Bus->Subscribe<FPP_BossDeadEvent>(
			[this](const FPP_BossDeadEvent& Event) { MarkBossDefeated(Event.BossName); });
	}
}

void APP_CrashGameMode::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPP_EventBus* Bus = UPP_EventBus::Get(this))
	{
		Bus->Unsubscribe(BossDeadHandle);
		BossDeadHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void APP_CrashGameMode::MarkBossDefeated(FName BossName)
{
	if (BossName.IsNone() || DefeatedBossNames.Contains(BossName))
	{
		return;
	}

	DefeatedBossNames.Add(BossName);
	UE_LOG(LogPP, Log, TEXT("[CrashGameMode] 记录已击败 Boss：%s（本局共 %d 个）"),
		*BossName.ToString(), DefeatedBossNames.Num());
}

void APP_CrashGameMode::SetDefeatedBossNames(const TSet<FName>& InNames)
{
	DefeatedBossNames = InNames;
	UE_LOG(LogPP, Log, TEXT("[CrashGameMode] 读档替换已击败 Boss 集合：%d 个"), DefeatedBossNames.Num());
}

void APP_CrashGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	APP_PlayerState* PS = NewPlayer->GetPlayerState<APP_PlayerState>();
	if (!PS) return;

	// 从 SessionManager 恢复大厅选择的角色ID
	FName CharId = NAME_None;

	// 优先用 PlayerController 上跨图携带的选择（大厅选人 / 读档恢复都会写这里）
	if (const APP_PlayerController* PC = Cast<APP_PlayerController>(NewPlayer))
	{
		CharId = PC->GetPendingCharacterId();
	}

	if (CharId.IsNone())
	{
		if (UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld()))
		{
			CharId = SM->GetPlayerCharacterSelection(PS->GetPlayerName());
			if (CharId.IsNone())
			{
				CharId = SM->GetLastCharacterSelection();
			}
		}
	}

	// 兜底：DataManager 角色表第一行（项目默认角色）
	if (CharId.IsNone())
	{
		if (UPP_DataManager* DM = UPP_DataManager::Get(GetWorld()))
		{
			CharId = DM->GetDefaultCharacterId();
		}
	}

	if (!CharId.IsNone())
	{
		PS->SetSelectedCharacterId(CharId);
		UE_LOG(LogPP, Log, TEXT("[CrashGameMode] PostLogin - Restored character selection for %s: %s"),
			*PS->GetPlayerName(), *CharId.ToString());
	}

	GrantAbilitiesForCharacter(PS);

	// 恢复选择后重新应用外观
	if (APP_PlayerCharacter* PlayerChar = Cast<APP_PlayerCharacter>(NewPlayer->GetPawn()))
	{
		PlayerChar->ApplyCharacterAppearance(PS->GetSelectedCharacterId());
	}

	// 主菜单"继续游戏"：存档在 travel 前已缓存，这里在 Pawn/属性初始化完成之后一次性应用
	if (NewPlayer->IsLocalController())
	{
		if (UPP_SaveSubsystem* SaveSubsystem = UPP_SaveSubsystem::Get(this))
		{
			if (SaveSubsystem->HasPendingLoad())
			{
				FString Error;
				if (SaveSubsystem->ApplyPendingToPlayer(NewPlayer, Error))
				{
					UE_LOG(LogPP, Log, TEXT("[CrashGameMode] PostLogin - 已应用待读档存档"));
				}
				else
				{
					UE_LOG(LogPP, Error, TEXT("[CrashGameMode] PostLogin - 应用读档失败：%s"), *Error);
				}
			}
		}
	}
}

AActor* APP_CrashGameMode::ChoosePlayerStart_Implementation(AController* Player)
{
	// 优先查找关卡中放置的真实 PlayerStart，跳过编辑器自动生成的 APlayerStartPIE
	// APlayerStartPIE 是编辑器 Play 时在摄像机位置创建的临时 Actor，会导致首次生成位置不在预期的 PlayerStart
	UWorld* World = GetWorld();
	if (World)
	{
		for (TActorIterator<APlayerStart> It(World); It; ++It)
		{
			APlayerStart* PS = *It;
			if (PS && !PS->IsA<APlayerStartPIE>())
			{
				UE_LOG(LogPP, Log, TEXT("[CrashGameMode] ChoosePlayerStart: %s at %s"),
					*PS->GetName(), *PS->GetActorLocation().ToString());
				return PS;
			}
		}
	}

	// 没找到真实 PlayerStart，回退到引擎默认行为（含 PIE Start）
	return Super::ChoosePlayerStart_Implementation(Player);
}

void APP_CrashGameMode::RestartPlayer(AController* NewPlayer)
{
	Super::RestartPlayer(NewPlayer);

	// 在 Pawn 生成并 Possess 之后，从 PlayerController 获取角色选择并应用外观
	APP_PlayerController* PC = Cast<APP_PlayerController>(NewPlayer);
	if (!PC) return;

	APP_PlayerCharacter* PlayerChar = Cast<APP_PlayerCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	// 优先从 PlayerController 的 PendingCharacterId（跨 ServerTravel 传递）
	FName CharId = PC->GetPendingCharacterId();

	// 后备：从 SessionManager 获取最后选择
	if (CharId.IsNone())
	{
		if (UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld()))
		{
			CharId = SM->GetLastCharacterSelection();
		}
	}

	// 兜底：DataManager 角色表第一行（项目默认角色）
	if (CharId.IsNone())
	{
		if (UPP_DataManager* DM = UPP_DataManager::Get(GetWorld()))
		{
			CharId = DM->GetDefaultCharacterId();
		}
	}

	if (!CharId.IsNone())
	{
		// 同步写入 PlayerState，确保 GrantAbilitiesForCharacter 等后续链路也能拿到
		if (APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>())
		{
			PS->SetSelectedCharacterId(CharId);
		}
		UE_LOG(LogPP, Log, TEXT("[CrashGameMode] RestartPlayer - Applying appearance: %s"), *CharId.ToString());
		PlayerChar->ApplyCharacterAppearance(CharId);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[CrashGameMode] RestartPlayer - No character selection found!"));
	}

	// 读档后的首次生成：落点改用存档里的篝火检查点（一次性消费，不影响死亡复活流程）
	if (UPP_SaveSubsystem* SaveSubsystem = UPP_SaveSubsystem::Get(this))
	{
		FTransform SpawnTransform;
		if (SaveSubsystem->ConsumePendingSpawnTransform(SpawnTransform))
		{
			PlayerChar->TeleportTo(SpawnTransform.GetLocation(), SpawnTransform.Rotator(), false, true);
			UE_LOG(LogPP, Log, TEXT("[CrashGameMode] 读档首次生成落在检查点：(%.0f, %.0f, %.0f)"),
				SpawnTransform.GetLocation().X, SpawnTransform.GetLocation().Y, SpawnTransform.GetLocation().Z);
		}
	}
}

void APP_CrashGameMode::GrantAbilitiesForCharacter(APP_PlayerState* PS)
{
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	FName CharId = PS->GetSelectedCharacterId();

	// 如果没有选择角色（直接进入战斗地图，非大厅流程），跳过
	if (CharId.IsNone()) return;

	// 授予角色的GAS能力
	if (const FCharacterAbilityList* AbilityList = CharacterAbilityMap.Find(CharId))
	{
		for (const auto& AbilityClass : AbilityList->Abilities)
		{
			if (!AbilityClass)
			{
				continue;
			}

			// 去重：角色的 StartupAbilities 可能已经授予过同一个技能，
			// 重复授予会让一次按键激活多份实例（日志"匹配能力数=2"），体力被多扣。
			if (ASC->FindAbilitySpecFromClass(AbilityClass))
			{
				UE_LOG(LogPP, Warning, TEXT("[PP_Skill] 跳过重复授予(GameMode): %s 已存在"),
					*AbilityClass->GetName());
				continue;
			}

			FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
			ASC->GiveAbility(AbilitySpec);
		}
	}

	// 应用角色的初始化GameplayEffect
	if (const TSubclassOf<UGameplayEffect>* InitEffectClass = CharacterInitEffectMap.Find(CharId))
	{
		if (*InitEffectClass)
		{
			FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
			EffectContext.AddSourceObject(PS);
			FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(*InitEffectClass, 1, EffectContext);
			if (SpecHandle.IsValid())
			{
				ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
			}
		}
	}
}

void APP_CrashGameMode::ReturnToLobby()
{
	UE_LOG(LogPP, Verbose, TEXT("[CrashGameMode] ReturnToLobby - ServerTravel to LobbyMap"));

	// 回大厅前自动保存一次（服务端写档；与同帧其它时机去重由 SaveSubsystem 负责）
	if (UPP_SaveSubsystem* SaveSubsystem = UPP_SaveSubsystem::Get(this))
	{
		FString SaveError;
		if (!SaveSubsystem->SaveLocalPlayer(SaveError))
		{
			UE_LOG(LogPP, Warning, TEXT("[CrashGameMode] 返回大厅前保存失败：%s"), *SaveError);
		}
	}

	// 使用 ProcessServerTravel 将所有玩家带回大厅（与 StartGame 保持一致）
	FString TravelURL = TEXT("/Game/PersonalProject/Maps/LobbyMap?Listen");
	ProcessServerTravel(*TravelURL);
}

void APP_CrashGameMode::RequestReturnToLobby_Implementation()
{
	ReturnToLobby();
}
