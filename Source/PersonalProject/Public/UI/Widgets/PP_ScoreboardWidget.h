// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "Components/PanelWidget.h"
#include "PP_ScoreboardWidget.generated.h"

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

/**
 * UPP_ScoreboardWidget - 计分板Widget
 * Tab键按住显示，展示所有玩家的击杀/死亡/得分
 */
UCLASS()
class PERSONALPROJECT_API UPP_ScoreboardWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_ScoreboardWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	/** 更新计分板数据 */
	UFUNCTION(BlueprintCallable, Category = "PP|Scoreboard")
	void UpdateScoreboard(const TArray<FScoreboardEntry>& Entries);

protected:

	/** 蓝图绑定 - 计分板条目容器 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> ScoreboardContainer;

	/** 计分板条目Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Scoreboard")
	TSubclassOf<UUserWidget> ScoreboardEntryClass;

	/** 蓝图实现事件 - 计分板更新 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Scoreboard Updated"))
	void BP_OnScoreboardUpdated(const TArray<FScoreboardEntry>& Entries);
};
