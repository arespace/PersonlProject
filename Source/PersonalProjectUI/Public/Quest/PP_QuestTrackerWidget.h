// 任务系统 — HUD 右侧任务追踪器 Widget
// 显示当前追踪任务的名称和各目标进度

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "Data/PP_QuestData.h"
#include "PP_QuestTrackerWidget.generated.h"

class UPanelWidget;
class UTextBlock;
class UPP_QuestComponent;

/**
 * 任务追踪器 Widget — HUD 右侧常驻显示
 *
 * 职责：
 *   1. 显示当前追踪的任务名称
 *   2. 列出各目标及实时进度（如 "击杀敌人 2/5"）
 *   3. 监听 OnObjectiveUpdated 实时刷新
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_QuestTrackerWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_QuestTrackerWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	/** 初始化：绑定 QuestComponent 事件 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void InitializeTracker(UPP_QuestComponent* InQuestComponent);

	/** 设置追踪的任务 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void TrackQuest(FName QuestID);

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
	UFUNCTION()
	void OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex);

	UFUNCTION()
	void OnQuestCompleted(FName QuestID);

	/** OnRep 触发的列表刷新（客户端也能收到） */
	UFUNCTION()
	void OnQuestsUpdated();

	/** 当前追踪的任务ID */
	UPROPERTY()
	FName TrackedQuestID;

	/** 缓存的 QuestComponent 引用 */
	UPROPERTY()
	TObjectPtr<UPP_QuestComponent> QuestComponent;
};
