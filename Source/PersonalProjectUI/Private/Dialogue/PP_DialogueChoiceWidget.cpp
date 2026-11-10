// NPC对话系统 — 对话结束选项面板实现
// 按钮可见性控制、点击回调上报 Server RPC

#include "Dialogue/PP_DialogueChoiceWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Dialogue/PP_DialogueSource.h"
#include "Dialogue/PP_DialogueRequester.h"
#include "Framework/PP_DataManager.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/PP_UIManager.h"

UPP_DialogueChoiceWidget::UPP_DialogueChoiceWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Modal;
	bConsumesAllInput = true;
}

void UPP_DialogueChoiceWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定按钮点击事件
	if (OfferQuestButton)  OfferQuestButton->OnClicked.AddDynamic(this, &ThisClass::OnOfferQuestClicked);
	if (SubmitQuestButton) SubmitQuestButton->OnClicked.AddDynamic(this, &ThisClass::OnSubmitQuestClicked);
	if (OpenShopButton)    OpenShopButton->OnClicked.AddDynamic(this, &ThisClass::OnOpenShopClicked);
	if (ContinueButton)    ContinueButton->OnClicked.AddDynamic(this, &ThisClass::OnContinueClicked);
	if (CloseButton)       CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnCloseClicked);
}

void UPP_DialogueChoiceWidget::SetupChoices(EPP_DialogueEndAction InAction, UObject* InOwningNPC, FName InDialogueID, APlayerController* InOwnerPC)
{
	CurrentAction = InAction;
	CurrentDialogueID = InDialogueID;
	OwningNPC = InOwningNPC;
	OwnerPC = InOwnerPC;

	// 默认全部隐藏（如蓝图提供了按钮）
	auto HideBtn = [](UButton* Btn)
	{
		if (Btn) Btn->SetVisibility(ESlateVisibility::Collapsed);
	};

	HideBtn(OfferQuestButton);
	HideBtn(SubmitQuestButton);
	HideBtn(OpenShopButton);
	HideBtn(ContinueButton);

	// 根据动作类型显示对应按钮
	switch (CurrentAction)
	{
	case EPP_DialogueEndAction::OfferQuest:
		if (OfferQuestButton) OfferQuestButton->SetVisibility(ESlateVisibility::Visible);
		break;
	case EPP_DialogueEndAction::SubmitQuest:
		if (SubmitQuestButton) SubmitQuestButton->SetVisibility(ESlateVisibility::Visible);
		break;
	case EPP_DialogueEndAction::OpenShop:
		if (OpenShopButton) OpenShopButton->SetVisibility(ESlateVisibility::Visible);
		break;
	case EPP_DialogueEndAction::ChainNextDialogue:
		if (ContinueButton) ContinueButton->SetVisibility(ESlateVisibility::Visible);
		break;
	default:
		break;
	}

	// 关闭按钮始终可见
	if (CloseButton) CloseButton->SetVisibility(ESlateVisibility::Visible);

	// 设置提示文字（如任务名）
	if (PromptText)
	{
		FText PromptContent = FText::GetEmpty();

		if (CurrentAction == EPP_DialogueEndAction::OfferQuest || CurrentAction == EPP_DialogueEndAction::SubmitQuest)
		{
			// 从对话数据表查找 RelatedQuestID
			FName RelatedQuestID = NAME_None;
			if (OwningNPC.IsValid() && OwningNPC->Implements<UPP_DialogueSource>())
			{
				if (UDataTable* DT = IPP_DialogueSource::Execute_QueryDialogueTable(OwningNPC.Get()))
				{
					if (const FPP_DialogueData* Row = DT->FindRow<FPP_DialogueData>(CurrentDialogueID, TEXT("SetupChoices-Prompt")))
					{
						RelatedQuestID = Row->RelatedQuestID;
					}
				}
			}

			// 从任务数据表查找任务名（统一走 DataManager 全局任务表）
			if (!RelatedQuestID.IsNone())
			{
				if (UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld()))
				{
					if (const FPP_QuestInfo* QuestInfo = DataManager->GetQuestInfo(RelatedQuestID))
					{
						PromptContent = QuestInfo->QuestName;
					}
				}
			}

			// Fallback：数据查找失败时，使用默认提示
			if (PromptContent.IsEmpty())
			{
				PromptContent = (CurrentAction == EPP_DialogueEndAction::OfferQuest)
					? FText::FromString(TEXT("接取任务"))
					: FText::FromString(TEXT("提交任务"));
			}
		}

		PromptText->SetText(PromptContent);
	}
}

void UPP_DialogueChoiceWidget::CloseChoices()
{
	// 通过 UIManager 统一弹出，输入模式由其 RecalculateInputMode → ApplyInputContext 负责
	bool bPoppedViaManager = false;
	if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
	{
		if (Mgr->IsWidgetInStack(this))
		{
			Mgr->PopWidget(this);
			bPoppedViaManager = true;
		}
	}
	if (!bPoppedViaManager)
	{
		RemoveFromParent();

		// 未经过栈时也要让 UIManager 重新计算输入模式
		if (UPP_UIManager* Mgr = UPP_UIManager::Get(this))
		{
			Mgr->RefreshInputMode();
		}
	}
}

void UPP_DialogueChoiceWidget::ReportAndClose(EPP_DialogueEndAction Action)
{
	if (OwningNPC.IsValid())
	{
		if (APlayerController* PC = OwnerPC.Get())
		{
			if (IPP_DialogueRequester* Requester = Cast<IPP_DialogueRequester>(PC))
			{
				IPP_DialogueRequester::Execute_ConfirmDialogueEndAction(PC, OwningNPC.Get(), CurrentDialogueID, Action);
			}
		}
	}

	CloseChoices();
}

void UPP_DialogueChoiceWidget::OnOfferQuestClicked()
{
	ReportAndClose(EPP_DialogueEndAction::OfferQuest);
}

void UPP_DialogueChoiceWidget::OnSubmitQuestClicked()
{
	ReportAndClose(EPP_DialogueEndAction::SubmitQuest);
}

void UPP_DialogueChoiceWidget::OnOpenShopClicked()
{
	ReportAndClose(EPP_DialogueEndAction::OpenShop);
}

void UPP_DialogueChoiceWidget::OnContinueClicked()
{
	ReportAndClose(EPP_DialogueEndAction::ChainNextDialogue);
}

void UPP_DialogueChoiceWidget::OnCloseClicked()
{
	ReportAndClose(EPP_DialogueEndAction::None);
}
