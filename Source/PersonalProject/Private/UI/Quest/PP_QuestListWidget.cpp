// 任务系统 — 任务日志面板 Widget 实现

#include "UI/Quest/PP_QuestListWidget.h"

#include "Components/PanelWidget.h"
#include "Components/Button.h"
#include "Quest/Component/PP_QuestComponent.h"
#include "UI/Quest/PP_QuestEntryWidget.h"

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

void UPP_QuestListWidget::InitializeQuestList(UPP_QuestComponent* InQuestComponent)
{
	QuestComponent = InQuestComponent;
	if (!QuestComponent) return;

	QuestComponent->OnQuestAccepted.AddDynamic(this, &UPP_QuestListWidget::OnQuestAccepted);
	QuestComponent->OnQuestCompleted.AddDynamic(this, &UPP_QuestListWidget::OnQuestCompleted);
	QuestComponent->OnQuestsUpdated.AddDynamic(this, &UPP_QuestListWidget::OnQuestsUpdated);

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
	if (!QuestListContainer || !QuestComponent) return;

	QuestListContainer->ClearChildren();

	if (!QuestEntryWidgetClass) return;

	TArray<FPP_ActiveQuest> Quests = QuestComponent->GetActiveQuests();
	for (const FPP_ActiveQuest& Quest : Quests)
	{
		UPP_QuestEntryWidget* Entry = CreateWidget<UPP_QuestEntryWidget>(GetOwningPlayer(), QuestEntryWidgetClass);
		if (Entry)
		{
			const FPP_QuestInfo* QuestInfo = QuestComponent->FindQuestInfo(Quest.QuestID);
			if (QuestInfo)
			{
				Entry->SetupEntry(Quest, *QuestInfo);
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
	if (!QuestListContainer || !QuestComponent) return;

	QuestListContainer->ClearChildren();

	if (!QuestEntryWidgetClass) return;

	TArray<FName> CompletedIDs = QuestComponent->GetCompletedQuests();
	for (const FName& QuestID : CompletedIDs)
	{
		UPP_QuestEntryWidget* Entry = CreateWidget<UPP_QuestEntryWidget>(GetOwningPlayer(), QuestEntryWidgetClass);
		if (Entry)
		{
			const FPP_QuestInfo* QuestInfo = QuestComponent->FindQuestInfo(QuestID);
			if (QuestInfo)
			{
				Entry->SetupCompletedEntry(QuestID, *QuestInfo);
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

void UPP_QuestListWidget::OnQuestAccepted(FName QuestID)
{
	RefreshQuestList();
}

void UPP_QuestListWidget::OnQuestCompleted(FName QuestID)
{
	RefreshQuestList();
}

void UPP_QuestListWidget::OnQuestsUpdated()
{
	RefreshQuestList();
}
