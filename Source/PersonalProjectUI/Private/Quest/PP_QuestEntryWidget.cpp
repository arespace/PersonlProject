// 任务系统 — 任务条目 Widget 实现

#include "Quest/PP_QuestEntryWidget.h"

#include "Components/TextBlock.h"
#include "Components/Button.h"

void UPP_QuestEntryWidget::SetupEntry(const FPP_ActiveQuest& ActiveQuest, const FPP_QuestInfo& QuestInfo)
{
	CachedQuestID = ActiveQuest.QuestID;

	// 显示任务名称（来自 DataTable）
	if (QuestNameText)
	{
		QuestNameText->SetText(QuestInfo.QuestName);
	}

	// 显示任务描述（来自 DataTable）
	if (QuestDescText)
	{
		QuestDescText->SetText(QuestInfo.Description);
	}

	if (StatusText)
	{
		// 检查是否所有目标都已完成
		bool bAllComplete = true;
		for (const FPP_QuestObjective& Obj : ActiveQuest.Objectives)
		{
			if (!Obj.bCompleted)
			{
				bAllComplete = false;
				break;
			}
		}
		StatusText->SetText(bAllComplete ? FText::FromString(TEXT("可提交")) : FText::FromString(TEXT("进行中")));
	}
}

void UPP_QuestEntryWidget::SetupCompletedEntry(FName QuestID, const FPP_QuestInfo& QuestInfo)
{
	CachedQuestID = QuestID;

	// 显示任务名称（来自 DataTable）
	if (QuestNameText)
	{
		QuestNameText->SetText(QuestInfo.QuestName);
	}

	// 显示任务描述（来自 DataTable）
	if (QuestDescText)
	{
		QuestDescText->SetText(QuestInfo.Description);
	}

	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("已完成")));
	}

	if (TrackButton)
	{
		TrackButton->SetVisibility(ESlateVisibility::Collapsed);
	}
}
