// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameMode.h"
#include "PP_CrashGameMode.generated.h"

class UPP_QuestEventBridge;

/** 角色能力列表包装结构（UHT不支持TArray作为TMap值类型） */
USTRUCT(BlueprintType)
struct FCharacterAbilityList
{
	GENERATED_BODY()

	/** 该角色拥有的GAS能力类列表 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|GameMode|CharacterAbilities")
	TArray<TSubclassOf<class UGameplayAbility>> Abilities;
};

/**
 * APP_CrashGameMode - 项目主GameMode
 * 负责配置HUD、默认Pawn、PlayerController等核心类
 * BP_CrashGameMode蓝图应以此类为父类，以便继承HUD绑定
 *
 * 大厅系统集成：
 * - PostLogin中读取PP_PlayerState::SelectedCharacterId
 * - 根据角色ID授予对应GAS能力和初始化效果
 */
UCLASS()
class PERSONALPROJECT_API APP_CrashGameMode : public AGameMode
{
	GENERATED_BODY()

public:
	APP_CrashGameMode();

	// ===== SeamlessTravel桥接 =====

	/** 玩家登录后，根据大厅选择的角色授予GAS能力 */
	virtual void PostLogin(APlayerController* NewPlayer) override;

	/** 玩家重生时应用角色外观（绕过蓝图 PossessedBy 覆盖问题） */
	virtual void RestartPlayer(AController* NewPlayer) override;

	/** 覆盖 ChoosePlayerStart：处理 PIE 首次生成时 PlayerStart 未注册的时序问题 */
	virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;

	/** 根据角色ID授予对应的GameplayAbility和初始GameplayEffect */
	void GrantAbilitiesForCharacter(class APP_PlayerState* PS);

	// ===== 角色能力配置（可在蓝图中编辑） =====

	/** 角色ID -> 该角色的GAS能力列表 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|GameMode|CharacterAbilities")
	TMap<FName, FCharacterAbilityList> CharacterAbilityMap;

	// ===== 返回大厅 =====

	/** 返回大厅 - 所有玩家一起通过 ServerTravel 返回 LobbyMap */
	UFUNCTION(BlueprintCallable, Category = "PP|Game")
	void ReturnToLobby();

	/** 角色ID -> 该角色的初始化GameplayEffect */
	UPROPERTY(EditDefaultsOnly, Category = "PP|GameMode|CharacterAbilities")
	TMap<FName, TSubclassOf<class UGameplayEffect>> CharacterInitEffectMap;

	// ===== 任务系统 =====

	/** 获取任务事件桥接器 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Quest")
	UPP_QuestEventBridge* GetQuestEventBridge() const { return QuestEventBridge; }

protected:
	/** 任务事件桥接器组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Quest")
	TObjectPtr<UPP_QuestEventBridge> QuestEventBridge;
};
