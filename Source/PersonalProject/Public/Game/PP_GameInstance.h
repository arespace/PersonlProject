// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "PP_GameInstance.generated.h"

/**
 * UPP_GameInstance - 项目自定义 GameInstance
 * 角色选择数据统一存储在 SessionManager（GameInstanceSubsystem）中，
 * 调用方直接使用 UPP_SessionManager，本类不再做转发。
 */
UCLASS()
class PERSONALPROJECT_API UPP_GameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	/** 便捷静态方法：从 World 获取 GameInstance 实例 */
	static UPP_GameInstance* Get(const UWorld* World);
};
