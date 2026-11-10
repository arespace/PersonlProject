
#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Session/PP_ScoreboardTypes.h"
#include "PP_ScoreboardRowWidget.generated.h"

/**
 * UPP_ScoreboardRowWidget - 计分板行 Widget 基类
 * C++ 负责创建行并写入条目数据，蓝图子类只需在 OnEntrySet 中把字段刷新到界面
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_ScoreboardRowWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 由 UPP_ScoreboardWidget 创建行时调用：缓存数据并通知蓝图刷新显示 */
	UFUNCTION(BlueprintCallable, Category = "PP|Scoreboard")
	void SetEntry(const FScoreboardEntry& InEntry);

protected:
	/** 蓝图实现事件 - 条目数据已设置，蓝图在此刷新名字/击杀/死亡/得分 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Entry Set"))
	void OnEntrySet(const FScoreboardEntry& InEntry);

	/** 当前条目数据（蓝图可直接读取） */
	UPROPERTY(BlueprintReadOnly, Category = "PP|Scoreboard")
	FScoreboardEntry Entry;
};
