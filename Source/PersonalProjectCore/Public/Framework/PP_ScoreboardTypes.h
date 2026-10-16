// 计分板数据结构（Core 层，供 PlayerController RPC 与 UI Widget 共用）

#pragma once

#include "CoreMinimal.h"
#include "PP_ScoreboardTypes.generated.h"

/** 计分板条目数据 */
USTRUCT(BlueprintType)
struct FScoreboardEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite, Category = "PP|Scoreboard")
	FString PlayerName;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Scoreboard")
	int32 Kills = 0;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Scoreboard")
	int32 Deaths = 0;

	UPROPERTY(BlueprintReadWrite, Category = "PP|Scoreboard")
	int32 Score = 0;
};
