// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_CrashGameMode.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "UI/PP_HUD.h"
#include "Player/PP_PlayerController.h"
#include "Characters/PP_PlayerCharacter.h"
#include "Player/PP_PlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "AbilitySystemComponent.h"
#include "Quest/Bridge/PP_QuestEventBridge.h"
#include "Game/PP_GameInstance.h"
#include "Game/PP_SessionManager.h"

APP_CrashGameMode::APP_CrashGameMode()
{
	// 绑定BP_HUD蓝图到GameMode
	static ConstructorHelpers::FClassFinder<AHUD> HUDClassFinder(TEXT("/Game/PersonalProject/UI/HUD/BP_HUD.BP_HUD_C"));
	if (HUDClassFinder.Succeeded())
	{
		HUDClass = HUDClassFinder.Class;
	}
	else
	{
		HUDClass = APP_HUD::StaticClass();
	}

	// 绑定BP_PP_PlayerController蓝图到GameMode
	static ConstructorHelpers::FClassFinder<APlayerController> PCClassFinder(TEXT("/Game/PersonalProject/Player/BP_PP_PlayerController.BP_PP_PlayerController_C"));
	if (PCClassFinder.Succeeded())
	{
		PlayerControllerClass = PCClassFinder.Class;
	}
	else
	{
		PlayerControllerClass = APP_PlayerController::StaticClass();
	}

	// 绑定BP_PP_PlayerCharacter蓝图到GameMode（DefaultPawnClass）
	static ConstructorHelpers::FClassFinder<APawn> PawnClassFinder(TEXT("/Game/PersonalProject/Characters/PlayerCharacter/BP_PP_PlayerCharacter.BP_PP_PlayerCharacter_C"));
	if (PawnClassFinder.Succeeded())
	{
		DefaultPawnClass = PawnClassFinder.Class;
	}
	else
	{
		DefaultPawnClass = APP_PlayerCharacter::StaticClass();
	}

	// 绑定BP_PP_PlayerState蓝图到GameMode
	static ConstructorHelpers::FClassFinder<APlayerState> PSClassFinder(TEXT("/Game/PersonalProject/Player/BP_PP_PlayerState.BP_PP_PlayerState_C"));
	if (PSClassFinder.Succeeded())
	{
		PlayerStateClass = PSClassFinder.Class;
	}
	else
	{
		PlayerStateClass = APP_PlayerState::StaticClass();
	}

	// 任务事件桥接器组件
	QuestEventBridge = CreateDefaultSubobject<UPP_QuestEventBridge>("QuestEventBridge");
}

void APP_CrashGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	APP_PlayerState* PS = NewPlayer->GetPlayerState<APP_PlayerState>();
	if (!PS) return;

	// 从 GameInstance 恢复大厅选择的角色ID（GameInstance 跨 SeamlessTravel 真正持久）
	// 同时也尝试 SessionManager（作为备用路径）
	UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld());
	if (GI)
	{
		FName CharId = GI->GetPlayerCharacterSelection(PS->GetPlayerName());


		if (!CharId.IsNone())
		{
			PS->SetSelectedCharacterId(CharId);
			UE_LOG(LogTemp, Log, TEXT("[CrashGameMode] PostLogin - Restored character selection from GameInstance for %s: %s"),
				*PS->GetPlayerName(), *CharId.ToString());
		}
	}

	// 备用：如果 GameInstance 没有，尝试从 SessionManager 读取
	if (PS->GetSelectedCharacterId().IsNone())
	{
		UPP_SessionManager* SM = UPP_SessionManager::Get(GetWorld());
		if (SM)
		{
			FName CharId = SM->ConsumePlayerCharacterSelection(PS->GetPlayerName());
			if (!CharId.IsNone())
			{
				PS->SetSelectedCharacterId(CharId);
				UE_LOG(LogTemp, Log, TEXT("[CrashGameMode] PostLogin - Restored character selection from SessionManager for %s: %s"),
					*PS->GetPlayerName(), *CharId.ToString());
			}
		}
	}

	GrantAbilitiesForCharacter(PS);

	// 恢复选择后重新应用外观
	if (APP_PlayerCharacter* PlayerChar = Cast<APP_PlayerCharacter>(NewPlayer->GetPawn()))
	{
		PlayerChar->ApplyCharacterAppearance(PS->GetSelectedCharacterId());
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
				UE_LOG(LogTemp, Log, TEXT("[CrashGameMode] ChoosePlayerStart: %s at %s"),
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
	// 这绕过了 BP_PP_PlayerCharacter 可能覆盖 PossessedBy 导致 C++ 代码不执行的问题
	APP_PlayerController* PC = Cast<APP_PlayerController>(NewPlayer);
	if (!PC) return;

	APP_PlayerCharacter* PlayerChar = Cast<APP_PlayerCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	FName CharId = PC->GetPendingCharacterId();



	// 后备1: GameInstance 的 PlayerName 匹配
	if (CharId.IsNone())
	{
		if (UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld()))
		{
			if (APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>())
			{
				CharId = GI->GetPlayerCharacterSelection(PS->GetPlayerName());
			}
		}
	}

	// 后备2: GameInstance 的 LastCharacterId（不依赖 PlayerName，最可靠）
	if (CharId.IsNone())
	{
		if (UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld()))
		{
			CharId = GI->GetLastCharacterSelection();
		}
	}

	if (!CharId.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[CrashGameMode] RestartPlayer - Applying appearance: %s"), *CharId.ToString());
		PlayerChar->ApplyCharacterAppearance(CharId);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[CrashGameMode] RestartPlayer - No character selection found!"));
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
			if (AbilityClass)
			{
				FGameplayAbilitySpec AbilitySpec(AbilityClass, 1);
				ASC->GiveAbility(AbilitySpec);
			}
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
	UE_LOG(LogTemp, Warning, TEXT("[CrashGameMode] ReturnToLobby - ServerTravel to LobbyMap"));

	// 使用 ProcessServerTravel 将所有玩家带回大厅（与 StartGame 保持一致）
	FString TravelURL = TEXT("/Game/PersonalProject/Maps/LobbyMap?Listen");
	ProcessServerTravel(*TravelURL);
}
