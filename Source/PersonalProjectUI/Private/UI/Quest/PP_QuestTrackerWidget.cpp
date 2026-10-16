// 任务系统 — HUD 右侧任务追踪器 Widget 实现

#include "UI/Quest/PP_QuestTrackerWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Layout/Margin.h"
#include "Quest/Component/PP_QuestComponent.h"

UPP_QuestTrackerWidget::UPP_QuestTrackerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Background;
}

void UPP_QuestTrackerWidget::InitializeTracker(UPP_QuestComponent* InQuestComponent)
{
	QuestComponent = InQuestComponent;
	if (!QuestComponent) return;

	QuestComponent->OnObjectiveUpdated.AddDynamic(this, &UPP_QuestTrackerWidget::OnObjectiveUpdated);
	QuestComponent->OnQuestCompleted.AddDynamic(this, &UPP_QuestTrackerWidget::OnQuestCompleted);
	QuestComponent->OnQuestsUpdated.AddDynamic(this, &UPP_QuestTrackerWidget::OnQuestsUpdated);

	// 默认追踪第一个活跃任务
	TArray<FPP_ActiveQuest> ActiveQuests = QuestComponent->GetActiveQuests();
	UE_LOG(LogTemp, Log, TEXT("[QuestTracker] InitializeTracker - ActiveQuests=%d"), ActiveQuests.Num());
	if (ActiveQuests.Num() > 0)
	{
		TrackQuest(ActiveQuests[0].QuestID);
	}
}

void UPP_QuestTrackerWidget::TrackQuest(FName QuestID)
{
	TrackedQuestID = QuestID;
	RefreshTracker();
}

void UPP_QuestTrackerWidget::RefreshTracker()
{
	if (!QuestComponent) return;

	if (TrackedQuestID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuestTracker] RefreshTracker - TrackedQuestID is None, clearing UI"));
		if (QuestTitleText)
		{
			QuestTitleText->SetText(FText::GetEmpty());
		}
		if (TrackerContainer)
		{
			TrackerContainer->ClearChildren();
		}
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[QuestTracker] RefreshTracker - Tracking %s, QuestTitleText=%s, TrackerContainer=%s"),
		*TrackedQuestID.ToString(),
		QuestTitleText ? TEXT("valid") : TEXT("NULL"),
		TrackerContainer ? TEXT("valid") : TEXT("NULL"));

	// 查找追踪的任务
	TArray<FPP_ActiveQuest> ActiveQuests = QuestComponent->GetActiveQuests();
	const FPP_ActiveQuest* TrackedQuest = nullptr;
	for (const FPP_ActiveQuest& Quest : ActiveQuests)
	{
		if (Quest.QuestID == TrackedQuestID)
		{
			TrackedQuest = &Quest;
			break;
		}
	}

	if (!TrackedQuest)
	{
		if (QuestTitleText)
		{
			QuestTitleText->SetText(FText::GetEmpty());
		}
		return;
	}

	// 更新标题
	if (QuestTitleText)
	{
		QuestTitleText->SetText(FText::FromName(TrackedQuest->QuestID));
	}

	// 更新目标进度文本（通过 TrackerContainer 内 TextBlock 子控件显示）
	// 简化实现：将所有目标进度拼接到标题下方
	if (TrackerContainer)
	{
		TrackerContainer->ClearChildren();

		int32 ObjectiveIndex = 0;
		for (const FPP_QuestObjective& Obj : TrackedQuest->Objectives)
		{
			UTextBlock* ObjText = NewObject<UTextBlock>(this);
			if (ObjText)
			{
				FString ProgressStr = FString::Printf(TEXT("%s %d/%d"),
					*Obj.Description.ToString(), Obj.CurrentCount, Obj.RequiredCount);
				ObjText->SetText(FText::FromString(ProgressStr));

				// 设置字体样式，防止重叠并保持与标题一致的大小
				FSlateFontInfo FontInfo = ObjText->GetFont();
				FontInfo.Size = 18;
				ObjText->SetFont(FontInfo);
				ObjText->SetColorAndOpacity(FSlateColor(FLinearColor::White));

				// 设置自动换行，避免文本过长溢出
				ObjText->SetAutoWrapText(true);

				TrackerContainer->AddChild(ObjText);

				// 如果容器是 VerticalBox，设置 Slot Padding 以提供间距
				if (UVerticalBoxSlot* VBSlot = Cast<UVerticalBoxSlot>(ObjText->Slot))
				{
					VBSlot->SetPadding(FMargin(0.f, 4.f, 0.f, 4.f));
					VBSlot->SetHorizontalAlignment(HAlign_Left);
					VBSlot->SetVerticalAlignment(VAlign_Top);
				}
				else if (UHorizontalBoxSlot* HBSlot = Cast<UHorizontalBoxSlot>(ObjText->Slot))
				{
					HBSlot->SetPadding(FMargin(4.f, 0.f, 4.f, 0.f));
					HBSlot->SetVerticalAlignment(VAlign_Center);
				}
				else if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(ObjText->Slot))
				{
					// CanvasPanel 不会自动排列子控件，需要手动设置位置
					CanvasSlot->SetAutoSize(true);
					CanvasSlot->SetPosition(FVector2D(10.f, 10.f + ObjectiveIndex * 30.f));
					CanvasSlot->SetAnchors(FAnchors(0.f, 0.f));
					CanvasSlot->SetAlignment(FVector2D(0.f, 0.f));
				}
				else if (UOverlaySlot* OverlaySlot = Cast<UOverlaySlot>(ObjText->Slot))
				{
					// Overlay 也不会自动排列，通过 Padding 实现垂直偏移
					OverlaySlot->SetPadding(FMargin(10.f, 10.f + ObjectiveIndex * 30.f, 10.f, 0.f));
					OverlaySlot->SetHorizontalAlignment(HAlign_Left);
					OverlaySlot->SetVerticalAlignment(VAlign_Top);
				}

				ObjectiveIndex++;
			}
		}
	}
}

void UPP_QuestTrackerWidget::OnObjectiveUpdated(FName QuestID, int32 ObjectiveIndex)
{
	if (QuestID == TrackedQuestID)
	{
		RefreshTracker();
	}
}

void UPP_QuestTrackerWidget::OnQuestCompleted(FName QuestID)
{
	if (QuestID == TrackedQuestID)
	{
		// 任务完成，自动切换到下一个活跃任务
		TrackedQuestID = NAME_None;
		if (QuestComponent)
		{
			TArray<FPP_ActiveQuest> ActiveQuests = QuestComponent->GetActiveQuests();
			if (ActiveQuests.Num() > 0)
			{
				TrackedQuestID = ActiveQuests[0].QuestID;
			}
		}
		RefreshTracker();
	}
}

void UPP_QuestTrackerWidget::OnQuestsUpdated()
{
	UE_LOG(LogTemp, Log, TEXT("[QuestTracker] OnQuestsUpdated - TrackedID=%s"), *TrackedQuestID.ToString());
	// 客户端收到 OnRep 后，如果没有追踪任何任务则自动追踪第一个
	if (TrackedQuestID.IsNone() && QuestComponent)
	{
		TArray<FPP_ActiveQuest> ActiveQuests = QuestComponent->GetActiveQuests();
		if (ActiveQuests.Num() > 0)
		{
			TrackedQuestID = ActiveQuests[0].QuestID;
			UE_LOG(LogTemp, Log, TEXT("[QuestTracker] OnQuestsUpdated - Auto-tracking %s"), *TrackedQuestID.ToString());
		}
	}
	RefreshTracker();
}
