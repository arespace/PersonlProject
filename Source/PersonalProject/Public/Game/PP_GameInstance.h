// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PP_GameInstance.generated.h"

/**
 * UPP_GameInstance - 项目自定义 GameInstance
 * 角色选择数据统一存储在 SessionManager（GameInstanceSubsystem）中，
 * 此类仅作为转发门面，避免数据多处缓存。
 */
UCLASS()
class PERSONALPROJECT_API UPP_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 便捷静态方法：从 World 获取 GameInstance 实例 */
	static UPP_GameInstance* Get(const UWorld* World);

	// ===== 角色选择桥接（转发到 SessionManager） =====

	/** 存入玩家的角色选择（大厅阶段调用） */
	void SetPlayerCharacterSelection(const FString& PlayerName, FName CharacterId);

	/** 读取玩家的角色选择（不清除，可多次读取） */
	FName GetPlayerCharacterSelection(const FString& PlayerName) const;

	/** 不依赖PlayerName，直接返回最后存入的角色选择 */
	FName GetLastCharacterSelection() const;

	/** 清除指定玩家的选择 */
	void ClearPlayerCharacterSelection(const FString& PlayerName);

	/** 清除所有玩家的选择 */
	void ClearAllPlayerCharacterSelections();
};
