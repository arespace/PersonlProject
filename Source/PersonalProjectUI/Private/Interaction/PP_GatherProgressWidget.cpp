// 采集系统 — 采集进度条Widget 实现

#include "Interaction/PP_GatherProgressWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPP_GatherProgressWidget::SetProgress(float InProgress)
{
	const float ClampedProgress = FMath::Clamp(InProgress, 0.0f, 1.0f);
	if (ProgressBar)
	{
		ProgressBar->SetPercent(ClampedProgress);
	}
}

void UPP_GatherProgressWidget::ShowWidget()
{
	SetVisibility(ESlateVisibility::HitTestInvisible);
	SetProgress(0.0f);
}

void UPP_GatherProgressWidget::HideWidget()
{
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPP_GatherProgressWidget::SetGatherItemName(const FText& InName)
{
	if (GatherItemNameText)
	{
		GatherItemNameText->SetText(InName);
	}
}
