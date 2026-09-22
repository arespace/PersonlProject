// 任务系统 — 任务条目 Widget
// 列表中单行任务条目，显示任务名称、描述、状态

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Quest/Data/PP_QuestData.h"
#include "PP_QuestEntryWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * 任务条目 Widget — 列表中的一行
 *
 * 职责：
 *   1. 显示任务名称和描述
 *   2. 显示任务状态（进行中/已完成）
 *   3. 提供追踪按钮（设置为 HUD 追踪目标）
 */
UCLASS()
class PERSONALPROJECT_API UPP_QuestEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 设置为活跃任务条目 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void SetupEntry(const FPP_ActiveQuest& ActiveQuest, const FPP_QuestInfo& QuestInfo);

	/** 设置为已完成任务条目 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void SetupCompletedEntry(FName QuestID, const FPP_QuestInfo& QuestInfo);

protected:
	// ===== 蓝图绑定控件 =====

	/** 任务名称文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestNameText;

	/** 任务描述文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestDescText;

	/** 任务状态文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> StatusText;

	/** 追踪按钮 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> TrackButton;

	/** 缓存的任务ID */
	UPROPERTY(BlueprintReadOnly, Category = "Quest|UI")
	FName CachedQuestID;
};
