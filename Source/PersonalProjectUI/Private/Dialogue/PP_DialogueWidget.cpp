// NPC对话系统 — 对话框 Widget 实现
// 逐行推进台词逻辑、NextButton 绑定与自动切换到选项面板

#include "Dialogue/PP_DialogueWidget.h"

#include "Components/Button.h"
#include "Components/PanelSlot.h"
#include "Components/TextBlock.h"
#include "Dialogue/PP_DialogueSource.h"
#include "Dialogue/PP_DialogueRequester.h"
#include "GameFramework/PlayerController.h"
#include "Shop/PP_ShopQuery.h"
#include "Dialogue/PP_DialogueChoiceWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Framework/PP_Log.h"

UPP_DialogueWidget::UPP_DialogueWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Modal;
	bConsumesAllInput = true;
}

void UPP_DialogueWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (NextButton)
	{
		NextButton->OnClicked.AddDynamic(this, &ThisClass::OnNextButtonClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.AddDynamic(this, &ThisClass::OnCloseButtonClicked);
	}
	// 输入模式由 UIManager 根据 Modal 层 + bConsumesAllInput 自动管理
}

FReply UPP_DialogueWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	// 按 E 键或空格/回车 — 统一走 OnNextButtonClicked（商人→打开商店, 普通NPC→推进对话）
	if (InKeyEvent.GetKey() == EKeys::E || InKeyEvent.GetKey() == EKeys::SpaceBar || InKeyEvent.GetKey() == EKeys::Enter)
	{
		OnNextButtonClicked();
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void UPP_DialogueWidget::NativeDestruct()
{
	if (NextButton)
	{
		NextButton->OnClicked.RemoveDynamic(this, &ThisClass::OnNextButtonClicked);
	}

	if (CloseButton)
	{
		CloseButton->OnClicked.RemoveDynamic(this, &ThisClass::OnCloseButtonClicked);
	}

	Super::NativeDestruct();
}

void UPP_DialogueWidget::SetupDialogue(const FPP_DialogueData& InData, UObject* InOwningNPC, APlayerController* InOwnerPC)
{
	CurrentData = InData;
	OwningNPC = InOwningNPC;
	OwnerPC = InOwnerPC;
	CurrentLineIndex = 0;

	// 设置说话者名字
	if (SpeakerNameText)
	{
		SpeakerNameText->SetText(CurrentData.SpeakerName);
	}

	// 根据 NPC 类型设置按钮文字
	RefreshButtonVisibility();

	// 渲染第一行台词
	RenderCurrentLine();

	// 广播事件
	if (CurrentData.Lines.Num() > 0)
	{
		OnLineAdvanced.Broadcast(0, CurrentData.Lines[0]);
	}
}

void UPP_DialogueWidget::AdvanceLine()
{
	CurrentLineIndex++;

	if (CurrentLineIndex >= CurrentData.Lines.Num())
	{
		// 所有台词读完，进入结束选项
		if (CurrentData.EndAction == EPP_DialogueEndAction::None)
		{
			// 无后续动作，直接关闭对话
			CloseDialogue();

			// 触发 PC 的对话结束委托（经 IPP_DialogueRequester 接口，UI 不直连 PC）
			if (APlayerController* PC = OwnerPC.Get())
			{
				if (IPP_DialogueRequester* Requester = Cast<IPP_DialogueRequester>(PC))
				{
					IPP_DialogueRequester::Execute_NotifyDialogueEnded(PC, CurrentData.DialogueID, CurrentData.RelatedQuestID);
				}
			}
		}
		else if (CurrentData.EndAction == EPP_DialogueEndAction::ChainNextDialogue)
		{
			// 链式对话：自动跳到下一段对话，不弹出选项面板
			UE_LOG(LogPP, Log, TEXT("[DialogueWidget] AdvanceLine - ChainNextDialogue: DialogueID=%s, NPC=%s"),
				*CurrentData.DialogueID.ToString(), OwningNPC.IsValid() ? *OwningNPC->GetName() : TEXT("null"));

			if (APlayerController* PC = OwnerPC.Get())
			{
				if (OwningNPC.IsValid() && Cast<IPP_DialogueRequester>(PC))
				{
					IPP_DialogueRequester::Execute_ConfirmDialogueEndAction(PC, OwningNPC.Get(), CurrentData.DialogueID, EPP_DialogueEndAction::ChainNextDialogue);
				}
				else
				{
					UE_LOG(LogPP, Warning, TEXT("[DialogueWidget] AdvanceLine - ChainNextDialogue failed: OwnerPC valid=%d, OwningNPC valid=%d"),
						OwnerPC.IsValid(), OwningNPC.IsValid());
				}
			}
			// 通过 UIManager 移除当前 Widget（新的 DialogueWidget 会被 ServerRPC → Client_OpenDialogueUI 创建并添加）
			if (UGameInstance* GI = GetGameInstance())
			{
				if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
				{
					if (Mgr->IsWidgetInStack(this))
					{
						Mgr->PopWidget(this);
					}
					else
					{
						RemoveFromParent();
					}
				}
			}
			else
			{
				RemoveFromParent();
			}
		}
		else
		{
			// OfferQuest / SubmitQuest / OpenShop → 弹出选项面板
			ShowEndChoices();
		}
		return;
	}

	// 渲染新台词
	RenderCurrentLine();

	// 广播推进事件
	if (CurrentData.Lines.IsValidIndex(CurrentLineIndex))
	{
		OnLineAdvanced.Broadcast(CurrentLineIndex, CurrentData.Lines[CurrentLineIndex]);
	}

	// 触发 PC 侧的推进委托（经接口转发）
	if (APlayerController* PC = OwnerPC.Get())
	{
		if (IPP_DialogueRequester* Requester = Cast<IPP_DialogueRequester>(PC))
		{
			IPP_DialogueRequester::Execute_NotifyDialogueLineAdvanced(PC, CurrentLineIndex);
		}
	}
}

void UPP_DialogueWidget::ShowEndChoices()
{
	// 先创建 ChoiceWidget 并通过 AddToViewport → NativeConstruct → UIManager.PushWidget
	// 确保输入模式在 DialogueWidget 移除前已正确切换
	if (!ChoiceWidgetClass)
	{
		UE_LOG(LogPP, Warning, TEXT("[DialogueWidget] ShowEndChoices - ChoiceWidgetClass not set, closing dialogue"));
		if (APlayerController* PC = OwnerPC.Get())
		{
			if (IPP_DialogueRequester* Requester = Cast<IPP_DialogueRequester>(PC))
			{
				IPP_DialogueRequester::Execute_NotifyDialogueEnded(PC, CurrentData.DialogueID, CurrentData.RelatedQuestID);
			}
		}
		CloseDialogue();
		return;
	}

	UPP_DialogueChoiceWidget* ChoiceWidget = CreateWidget<UPP_DialogueChoiceWidget>(GetWorld(), ChoiceWidgetClass);
	if (!ChoiceWidget)
	{
		UE_LOG(LogPP, Error, TEXT("[DialogueWidget] ShowEndChoices - Failed to create ChoiceWidget"));
		return;
	}

	ChoiceWidget->AddToViewport(100);
	ChoiceWidget->SetupChoices(
		CurrentData.EndAction,
		OwningNPC.Get(),
		CurrentData.DialogueID,
		OwnerPC.Get()
	);

	// 通过 UIManager 移除对话框自身，避免 RemoveFromParent 与 PopWidget 重复调用
	// 导致 Slate 输入状态混乱
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
		{
			if (Mgr->IsWidgetInStack(this))
			{
				Mgr->PopWidget(this);
				return;
			}
		}
	}
	RemoveFromParent();
}

void UPP_DialogueWidget::CloseDialogue()
{
	APlayerController* PC = OwnerPC.Get();

	// 通过 UIManager 统一弹出，避免 RemoveFromParent 与 PopWidget 重复调用
	bool bPoppedViaManager = false;
	if (UGameInstance* GI = GetGameInstance())
	{
		if (UPP_UIManager* Mgr = GI->GetSubsystem<UPP_UIManager>())
		{
			if (Mgr->IsWidgetInStack(this))
			{
				Mgr->PopWidget(this);
				bPoppedViaManager = true;
			}
		}
	}
	if (!bPoppedViaManager)
	{
		RemoveFromParent();
	}

	// 强制恢复游戏输入（在 RecalculateInputMode 之后执行，确保最终状态正确）
	if (PC)
	{
		PC->ResetIgnoreInputFlags();
		PC->SetInputMode(FInputModeGameOnly());
		PC->bShowMouseCursor = false;
		FSlateApplication::Get().SetUserFocusToGameViewport(0);
		FSlateApplication::Get().ReleaseAllPointerCapture();
	}
}

void UPP_DialogueWidget::RenderCurrentLine()
{
	if (!LineText) return;

	if (CurrentData.Lines.IsValidIndex(CurrentLineIndex))
	{
		LineText->SetText(CurrentData.Lines[CurrentLineIndex]);
	}
	else
	{
		LineText->SetText(FText::GetEmpty());
	}
}

void UPP_DialogueWidget::OnNextButtonClicked()
{
	UObject* NPC = OwningNPC.Get();
	UObject* Shop = (NPC && NPC->Implements<UPP_DialogueSource>()) ? IPP_DialogueSource::Execute_QueryShopSource(NPC) : nullptr;
	// 只有 ShopComponent 存在 且 配置了商品数据表 才算真正的商人
	const bool bIsMerchant = (Shop && Shop->Implements<UPP_ShopQuery>() && IPP_ShopQuery::Execute_QueryShopItems(Shop).Num() > 0);

	if (bIsMerchant)
	{
		// 商人：打开商店
		if (!OwnerPC.IsValid() || !NPC || !IsValid(NPC))
		{
			UE_LOG(LogPP, Warning, TEXT("[DialogueWidget] OnNextButtonClicked - OpenShop failed: NPC or PC invalid"));
			return;
		}
		UE_LOG(LogPP, Log, TEXT("[DialogueWidget] OnNextButtonClicked - OpenShop for NPC '%s'"), *NPC->GetName());
		if (APlayerController* PC = OwnerPC.Get())
		{
			if (IPP_DialogueRequester* Requester = Cast<IPP_DialogueRequester>(PC))
			{
				IPP_DialogueRequester::Execute_ConfirmDialogueEndAction(PC, NPC, CurrentData.DialogueID, EPP_DialogueEndAction::OpenShop);
			}
		}
		RemoveFromParent();
	}
	else
	{
		// 普通 NPC：推进对话
		AdvanceLine();
	}
}

void UPP_DialogueWidget::RefreshButtonVisibility()
{
	if (!NextButton)
	{
		return;
	}

	UObject* NPC = OwningNPC.Get();
	UObject* Shop = (NPC && NPC->Implements<UPP_DialogueSource>()) ? IPP_DialogueSource::Execute_QueryShopSource(NPC) : nullptr;
	// 只有 ShopComponent 存在 且 配置了商品数据表（有商品） 才算真正的商人
	const bool bIsMerchant = (Shop && Shop->Implements<UPP_ShopQuery>() && IPP_ShopQuery::Execute_QueryShopItems(Shop).Num() > 0);

	// 修改 NextButton 的显示文字
	if (UPanelSlot* PanelSlot = NextButton->GetContentSlot())
	{
		if (UTextBlock* ButtonText = Cast<UTextBlock>(PanelSlot->Content))
		{
			ButtonText->SetText(bIsMerchant
				? FText::FromString(TEXT("打开商店"))
				: FText::FromString(TEXT("继续")));
		}
	}
}

void UPP_DialogueWidget::OnCloseButtonClicked()
{
	CloseDialogue();
}
