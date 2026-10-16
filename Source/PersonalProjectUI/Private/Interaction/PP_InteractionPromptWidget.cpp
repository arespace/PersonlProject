// 采集系统 — 交互提示Widget 实现

#include "Interaction/PP_InteractionPromptWidget.h"

#include "Components/TextBlock.h"

void UPP_InteractionPromptWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 应用默认文本
	if (PromptText && !DefaultPromptText.IsEmpty())
	{
		PromptText->SetText(DefaultPromptText);
	}

	// 默认隐藏
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPP_InteractionPromptWidget::SetPromptText(const FText& InText)
{
	if (PromptText)
	{
		PromptText->SetText(InText);
	}
}

void UPP_InteractionPromptWidget::ShowWidget()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
}

void UPP_InteractionPromptWidget::HideWidget()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
