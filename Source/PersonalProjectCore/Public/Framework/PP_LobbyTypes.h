// 大厅共享数据结构（Core 层，供主玩法 RPC 与 UI Widget 共用）
// 从 Player/PP_LobbyPlayerState.h 抽出：UI 模块只需要结构体，无需依赖玩法层 PlayerState 类

#pragma once

#include "CoreMinimal.h"
#include "PP_LobbyTypes.generated.h"

/** 大厅玩家信息（用于UI显示和网络同步） */
USTRUCT(BlueprintType)
struct FLobbyPlayerInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	bool bIsReady = false;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	FName SelectedCharacterId;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	int32 PlayerIndex = -1;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	bool bIsHost = false;
};

/** 房间设置 */
USTRUCT(BlueprintType)
struct FRoomSettings
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	FString MapName = TEXT("PPMap");

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	int32 MaxPlayers = 4;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Lobby")
	FString GameModeName = TEXT("Crash");
};
