// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_CrashGameMode.h"
#include "Engine/PlayerStartPIE.h"
#include "EngineUtils.h"
#include "UI/PP_HUD.h"
#include "Player/PP_PlayerController.h"
#include "Characters/PP_PlayerCharacter.h"
#include "Player/PP_PlayerState.h"
#include "AbilitySystemComponent.h"
#include "Quest/Bridge/PP_QuestEventBridge.h"
#include "Game/PP_GameInstance.h"
#include "Game/PP_SessionManager.h"

APP_CrashGameMode::APP_CrashGameMode()
{
	// 设置 C++ 基类作为默认值，蓝图（BP_CrashGameMode）中可覆盖为对应的蓝图类
	HUDClass = APP_HUD::StaticClass();
	PlayerControllerClass = APP_PlayerController::StaticClass();
	DefaultPawnClass = APP_PlayerCharacter::StaticClass();
	PlayerStateClass = APP_PlayerState::StaticClass();

	// 任务事件桥接器组件
	QuestEventBridge = CreateDefaultSubobject<UPP_QuestEventBridge>("QuestEventBridge");
}

void APP_CrashGameMode::PostLogin(APlayerController* NewPlayer)
{
	Super::PostLogin(NewPlayer);

	APP_PlayerState* PS = NewPlayer->GetPlayerState<APP_PlayerState>();
	if (!PS) return;

	// 从 GameInstance（转发到 SessionManager）恢复大厅选择的角色ID
	UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld());
	if (GI)
	{
		FName CharId = GI->GetPlayerCharacterSelection(PS->GetPlayerName());
		if (CharId.IsNone())
		{
			// 后备：不依赖 PlayerName 的最后选择
			CharId = GI->GetLastCharacterSelection();
		}

		if (!CharId.IsNone())
		{
			PS->SetSelectedCharacterId(CharId);
			UE_LOG(LogTemp, Log, TEXT("[CrashGameMode] PostLogin - Restored character selection for %s: %s"),
				*PS->GetPlayerName(), *CharId.ToString());
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
	APP_PlayerController* PC = Cast<APP_PlayerController>(NewPlayer);
	if (!PC) return;

	APP_PlayerCharacter* PlayerChar = Cast<APP_PlayerCharacter>(PC->GetPawn());
	if (!PlayerChar) return;

	// 优先从 PlayerController 的 PendingCharacterId（跨 ServerTravel 传递）
	FName CharId = PC->GetPendingCharacterId();

	// 后备：从 GameInstance（转发到 SessionManager）获取最后选择
	if (CharId.IsNone())
	{
		if (UPP_GameInstance* GI = UPP_GameInstance::Get(GetWorld()))
		{
			CharId = GI->GetLastCharacterSelection();
		}
	}

	if (!CharId.IsNone())
	{
		UE_LOG(LogTemp, Log, TEXT("[CrashGameMode] RestartPlayer - Applying appearance: %s"), *CharId.ToString());
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
