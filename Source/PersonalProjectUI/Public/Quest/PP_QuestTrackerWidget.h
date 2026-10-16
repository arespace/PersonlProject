// 任务系统 — HUD 右侧任务追踪器 Widget
// 显示当前追踪任务的名称和各目标进度

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "Data/PP_QuestData.h"
#include "PP_QuestTrackerWidget.generated.h"

class UPanelWidget;
class UTextBlock;

/**
 * 任务追踪器 Widget — HUD 右侧常驻显示
 *
 * 职责：
 *   1. 显示当前追踪的任务名称
 *   2. 列出各目标及实时进度（如 "击杀敌人 2/5"）
 *   3. 订阅 EventBus 的 FPP_QuestsUpdatedEvent 实时刷新
 *
 * 解耦说明：不再直接持有 UPP_QuestComponent，仅通过 IPP_QuestQuery 接口读取数据。
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_QuestTrackerWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_QuestTrackerWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 初始化：绑定任务数据源（须实现 IPP_QuestQuery 接口）并订阅任务变更事件 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void InitializeTracker(UObject* InQuestSource);

	/** 刷新追踪器显示 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void RefreshTracker();

protected:
	// ===== 蓝图绑定控件 =====

	/** 追踪目标列表容器 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> TrackerContainer;

	/** 任务标题文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> QuestTitleText;

private:
	/** EventBus 任务列表变更回调 */
	void HandleQuestsUpdated(const struct FPP_QuestsUpdatedEvent& Event);

	/** 当前追踪的任务ID */
	UPROPERTY()
	FName TrackedQuestID;

	/** 任务数据源（实现 IPP_QuestQuery 接口的对象，通常是 PlayerState 上的任务组件） */
	UPROPERTY()
	TObjectPtr<UObject> QuestSource;
};
