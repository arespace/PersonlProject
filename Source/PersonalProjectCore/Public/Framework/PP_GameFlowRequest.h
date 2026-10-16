// 游戏流程请求接口 — UI 模块 → 主玩法 GameMode 的边界
// 解耦：暂停菜单等 Widget 需要请求「返回大厅」，但不应 include 主玩法 GameMode 头文件，
// 改为通过本接口请求，由 APP_CrashGameMode 实现。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PP_GameFlowRequest.generated.h"

UINTERFACE(BlueprintType)
class UPP_GameFlowRequest : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_GameFlowRequest — 游戏流程请求接口
 * 实现方：APP_CrashGameMode（主玩法模块）
 * 调用方：UPP_PauseMenuWidget（UI 模块）
 */
class PERSONALPROJECTCORE_API IPP_GameFlowRequest
{
	GENERATED_BODY()

public:
	/** 请求返回大厅（实现方内部触发 ServerTravel） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Game")
	void RequestReturnToLobby();
};
