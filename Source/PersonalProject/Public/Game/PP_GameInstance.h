// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PP_GameInstance.generated.h"

/**
 * UPP_GameInstance - 项目自定义 GameInstance
 * 跨 SeamlessTravel 持久化玩家角色选择等数据
 * GameInstance 生命周期跟随整个游戏进程，不随地图切换销毁
 */
UCLASS()
class PERSONALPROJECT_API UPP_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 便捷静态方法：从 World 获取 GameInstance 实例 */
	static UPP_GameInstance* Get(const UWorld* World);

	// ===== 角色选择桥接（大厅 → 战斗关卡） =====

	/** 存入玩家的角色选择（大厅阶段调用） */
	void SetPlayerCharacterSelection(const FString& PlayerName, FName CharacterId);

	/** 取出玩家的角色选择（战斗关卡调用，取出后不清除，可多次读取） */
	FName GetPlayerCharacterSelection(const FString& PlayerName) const;

	/** 不依赖PlayerName，直接返回最后存入的角色选择（跨地图PlayerName变化时的后备方案） */
	FName GetLastCharacterSelection() const { return LastCharacterId; }

	/** 清除指定玩家的选择 */
	void ClearPlayerCharacterSelection(const FString& PlayerName);

	/** 清除所有玩家的选择 */
	void ClearAllPlayerCharacterSelections();

private:
	/** 玩家角色选择缓存：PlayerName → CharacterId */
	UPROPERTY()
	TMap<FString, FName> PlayerCharacterSelections;

	/** 最后存入的角色选择（不依赖PlayerName匹配的后备方案） */
	FName LastCharacterId;
};
