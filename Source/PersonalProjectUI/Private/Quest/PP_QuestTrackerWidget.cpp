// 任务系统 — HUD 右侧任务追踪器 Widget 实现

#include "Quest/PP_QuestTrackerWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/OverlaySlot.h"
#include "Layout/Margin.h"
#include "Quest/PP_QuestQuery.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "Framework/PP_Log.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"

UPP_QuestTrackerWidget::UPP_QuestTrackerWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Background;
}

void UPP_QuestTrackerWidget::InitializeTracker(UObject* InQuestSource)
{
	if (!InQuestSource || !InQuestSource->Implements<UPP_QuestQuery>())
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestTracker] InitializeTracker - 数据源未实现 IPP_QuestQuery 接口"));
		return;
	}

	QuestSource = InQuestSource;

	// 订阅任务列表变更事件（EventBus，替代原对 QuestComponent 委托的直连绑定）
	if (UPP_EventBus* EventBus = GetGameInstance()->GetSubsystem<UPP_EventBus>())
	{
		QuestsUpdatedHandle = EventBus->Subscribe<FPP_QuestsUpdatedEvent>(
			[this](const FPP_QuestsUpdatedEvent& Event)
			{
				HandleQuestsUpdated(Event);
			});
	}

	RefreshTracker();
}

void UPP_QuestTrackerWidget::NativeDestruct()
{
	if (UPP_EventBus* EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPP_EventBus>() : nullptr)
	{
		EventBus->Unsubscribe(QuestsUpdatedHandle);
	}
	QuestsUpdatedHandle.Reset();

	Super::NativeDestruct();
}

void UPP_QuestTrackerWidget::TrackQuest(FName QuestID)
{
	TrackedQuestID = QuestID;
	RefreshTracker();
}

void UPP_QuestTrackerWidget::RefreshTracker()
{
	if (!QuestSource)
	{
		return;
	}

	TArray<FPP_ActiveQuest> ActiveQuests = IPP_QuestQuery::Execute_QueryActiveQuests(QuestSource);

	// 追踪任务已失效（未设置/已完成/已放弃）时，自动切换到第一个活跃任务
	const bool bTrackedStillActive = !TrackedQuestID.IsNone() && ActiveQuests.ContainsByPredicate(
		[this](const FPP_ActiveQuest& Quest) { return Quest.QuestID == TrackedQuestID; });

	if (!bTrackedStillActive)
	{
		TrackedQuestID = (ActiveQuests.Num() > 0) ? ActiveQuests[0].QuestID : NAME_None;
		UE_LOG(LogPP, Log, TEXT("[QuestTracker] RefreshTracker - Auto-track '%s' (ActiveQuests=%d)"),
			*TrackedQuestID.ToString(), ActiveQuests.Num());
	}

	if (TrackedQuestID.IsNone())
	{
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

	// 查找追踪的任务
	const FPP_ActiveQuest* TrackedQuest = ActiveQuests.FindByPredicate(
		[this](const FPP_ActiveQuest& Quest) { return Quest.QuestID == TrackedQuestID; });

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

void UPP_QuestTrackerWidget::HandleQuestsUpdated(const FPP_QuestsUpdatedEvent& Event)
{
	// 只处理本地玩家的任务变更（EventBus 是 GameInstance 级，可能收到其他玩家的广播）
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}

	RefreshTracker();
}
