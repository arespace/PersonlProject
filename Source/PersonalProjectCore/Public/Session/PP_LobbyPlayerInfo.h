// 大厅玩家状态只读接口 — UI 模块 → 主玩法 PlayerState 的边界
// 解耦：大厅 Widget 需要读取「是否房主/是否准备/玩家序号」，但不应 include 主玩法 PlayerState 头文件，
// 改为通过本接口读取，由 APP_LobbyPlayerState 实现。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PP_LobbyPlayerInfo.generated.h"

UINTERFACE(BlueprintType)
class UPP_LobbyPlayerInfo : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_LobbyPlayerInfo — 大厅玩家状态只读查询接口
 * 实现方：APP_LobbyPlayerState（主玩法模块）
 * 调用方：UPP_LobbyWidget（UI 模块）
 *
 * 说明：接口为 BlueprintNativeEvent，故不能加 const 限定符。
 */
class PERSONALPROJECTCORE_API IPP_LobbyPlayerInfo
{
	GENERATED_BODY()

public:
	/** 是否为房主 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	bool IsLobbyHost();

	/** 是否已准备 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	bool IsLobbyReady();

	/** 玩家序号（-1 表示未分配） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Lobby")
	int32 GetLobbyPlayerIndex();
};
