// 任务系统 — 任务日志面板 Widget 实现

#include "Quest/PP_QuestListWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Quest/PP_QuestQuery.h"
#include "Quest/PP_QuestEntryWidget.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameInstance.h"

UPP_QuestListWidget::UPP_QuestListWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Overlay;
	bConsumesAllInput = true;
}

void UPP_QuestListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ActiveTab)
	{
		ActiveTab->OnClicked.AddDynamic(this, &UPP_QuestListWidget::OnActiveTabClicked);
	}
	if (CompletedTab)
	{
		CompletedTab->OnClicked.AddDynamic(this, &UPP_QuestListWidget::OnCompletedTabClicked);
	}
}

void UPP_QuestListWidget::NativeDestruct()
{
	if (UPP_EventBus* EventBus = GetGameInstance() ? GetGameInstance()->GetSubsystem<UPP_EventBus>() : nullptr)
	{
		EventBus->Unsubscribe(QuestsUpdatedHandle);
	}
	QuestsUpdatedHandle.Reset();

	Super::NativeDestruct();
}

void UPP_QuestListWidget::InitializeQuestList(UObject* InQuestSource)
{
	if (!InQuestSource || !InQuestSource->Implements<UPP_QuestQuery>())
	{
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

	RefreshQuestList();
}

void UPP_QuestListWidget::RefreshQuestList()
{
	if (bShowingActive)
	{
		ShowActiveQuests();
	}
	else
	{
		ShowCompletedQuests();
	}
}

void UPP_QuestListWidget::ShowActiveQuests()
{
	bShowingActive = true;
	if (!QuestListContainer || !QuestSource) return;

	QuestListContainer->ClearChildren();

	if (!QuestEntryWidgetClass) return;

	TArray<FPP_ActiveQuest> Quests = IPP_QuestQuery::Execute_QueryActiveQuests(QuestSource);
	for (const FPP_ActiveQuest& Quest : Quests)
	{
		UPP_QuestEntryWidget* Entry = CreateWidget<UPP_QuestEntryWidget>(GetOwningPlayer(), QuestEntryWidgetClass);
		if (Entry)
		{
			FPP_QuestInfo QuestInfo;
			if (IPP_QuestQuery::Execute_QueryQuestInfo(QuestSource, Quest.QuestID, QuestInfo))
			{
				Entry->SetupEntry(Quest, QuestInfo);
			}
			else
			{
				// 找不到配置时用默认值
				FPP_QuestInfo FallbackInfo;
				FallbackInfo.QuestID = Quest.QuestID;
				FallbackInfo.QuestName = FText::FromName(Quest.QuestID);
				FallbackInfo.Description = FText::FromString(TEXT("（任务配置未找到）"));
				Entry->SetupEntry(Quest, FallbackInfo);
			}
			QuestListContainer->AddChild(Entry);
		}
	}
}

void UPP_QuestListWidget::ShowCompletedQuests()
{
	bShowingActive = false;
	if (!QuestListContainer || !QuestSource) return;

	QuestListContainer->ClearChildren();

	if (!QuestEntryWidgetClass) return;

	TArray<FName> CompletedIDs = IPP_QuestQuery::Execute_QueryCompletedQuests(QuestSource);
	for (const FName& QuestID : CompletedIDs)
	{
		UPP_QuestEntryWidget* Entry = CreateWidget<UPP_QuestEntryWidget>(GetOwningPlayer(), QuestEntryWidgetClass);
		if (Entry)
		{
			FPP_QuestInfo QuestInfo;
			if (IPP_QuestQuery::Execute_QueryQuestInfo(QuestSource, QuestID, QuestInfo))
			{
				Entry->SetupCompletedEntry(QuestID, QuestInfo);
			}
			else
			{
				FPP_QuestInfo FallbackInfo;
				FallbackInfo.QuestID = QuestID;
				FallbackInfo.QuestName = FText::FromName(QuestID);
				FallbackInfo.Description = FText::FromString(TEXT("（任务配置未找到）"));
				Entry->SetupCompletedEntry(QuestID, FallbackInfo);
			}
			QuestListContainer->AddChild(Entry);
		}
	}
}

void UPP_QuestListWidget::OnActiveTabClicked()
{
	ShowActiveQuests();
}

void UPP_QuestListWidget::OnCompletedTabClicked()
{
	ShowCompletedQuests();
}

void UPP_QuestListWidget::HandleQuestsUpdated(const FPP_QuestsUpdatedEvent& Event)
{
	// 只处理本地玩家的任务变更（EventBus 是 GameInstance 级，可能收到其他玩家的广播）
	if (Event.Player && Event.Player != GetOwningPlayer())
	{
		return;
	}

	RefreshQuestList();
}
