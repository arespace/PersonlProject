// 任务系统 — 任务日志面板 Widget
// 显示玩家的活跃任务和已完成任务列表，支持切换标签

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "Data/PP_QuestData.h"
#include "PP_QuestListWidget.generated.h"

class UPanelWidget;
class UButton;

/**
 * 任务日志面板 Widget
 *
 * 职责：
 *   1. 显示活跃/已完成任务列表（通过标签切换）
 *   2. 订阅 EventBus 的 FPP_QuestsUpdatedEvent 自动刷新
 *   3. 为每个任务条目创建 PP_QuestEntryWidget 子项
 *
 * 解耦说明：不再直接持有 UPP_QuestComponent，仅通过 IPP_QuestQuery 接口读取数据。
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_QuestListWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_QuestListWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 初始化：绑定任务数据源（须实现 IPP_QuestQuery 接口）并订阅任务变更事件 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void InitializeQuestList(UObject* InQuestSource);

	/** 刷新当前显示的任务列表 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void RefreshQuestList();

	/** 显示活跃任务 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void ShowActiveQuests();

	/** 显示已完成任务 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void ShowCompletedQuests();

protected:
	virtual void NativeConstruct() override;

	// ===== 蓝图绑定控件 =====

	/** 任务条目容器 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> QuestListContainer;

	/** 活跃任务标签按钮 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ActiveTab;

	/** 已完成任务标签按钮 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CompletedTab;

	/** 任务条目 Widget 类（蓝图中配置） */
	UPROPERTY(EditDefaultsOnly, Category = "Quest|UI")
	TSubclassOf<class UPP_QuestEntryWidget> QuestEntryWidgetClass;

private:
	/** EventBus 任务列表变更回调 */
	void HandleQuestsUpdated(const struct FPP_QuestsUpdatedEvent& Event);

	UFUNCTION()
	void OnActiveTabClicked();

	UFUNCTION()
	void OnCompletedTabClicked();

	/** 当前是否显示活跃任务（false=已完成） */
	bool bShowingActive = true;

	/** 任务数据源（实现 IPP_QuestQuery 接口的对象，通常是 PlayerState 上的任务组件） */
	UPROPERTY()
	TObjectPtr<UObject> QuestSource;
};
