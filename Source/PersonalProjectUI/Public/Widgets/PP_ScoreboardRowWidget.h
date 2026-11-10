
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/PP_ScoreboardTypes.h"
#include "PP_ScoreboardRowWidget.generated.h"

class UTextBlock;

/**
 * UPP_ScoreboardRowWidget - 计分板行 Widget 基类
 * C++ 负责创建行、写入条目数据并填充默认文本，蓝图子类只需摆放控件
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_ScoreboardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 由 UPP_ScoreboardWidget 创建行时调用：缓存数据、填充文本并通知蓝图做额外定制 */
	UFUNCTION(BlueprintCallable, Category = "PP|Scoreboard")
	void SetEntry(const FScoreboardEntry& InEntry);

protected:
	/** 蓝图实现事件 - 默认文本已由 C++ 填好，蓝图可在此做换色、插图标等额外定制 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Entry Set"))
	void OnEntrySet(const FScoreboardEntry& InEntry);

	/** 当前条目数据（蓝图可直接读取） */
	UPROPERTY(BlueprintReadOnly, Category = "PP|Scoreboard")
	FScoreboardEntry Entry;

	// 以下文本控件由蓝图按固定名字绑定，未绑定则跳过对应字段
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Name;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Kills;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Deaths;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> Text_Score;
};
