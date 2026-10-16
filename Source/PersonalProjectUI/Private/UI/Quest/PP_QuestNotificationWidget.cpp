// 任务系统 — 浮动通知 Widget 实现

#include "UI/Quest/PP_QuestNotificationWidget.h"

#include "Components/TextBlock.h"
#include "TimerManager.h"

void UPP_QuestNotificationWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 初始隐藏
	SetVisibility(ESlateVisibility::Collapsed);
}

void UPP_QuestNotificationWidget::ShowNotification(FText Message, float Duration)
{
	if (NotificationText)
	{
		NotificationText->SetText(Message);
	}

	SetVisibility(ESlateVisibility::HitTestInvisible);

	// 设置定时器自动隐藏
	UWorld* World = GetWorld();
	if (World)
	{
		World->GetTimerManager().ClearTimer(HideTimerHandle);
		World->GetTimerManager().SetTimer(HideTimerHandle, this,
			&UPP_QuestNotificationWidget::HideNotification, Duration, false);
	}
}

void UPP_QuestNotificationWidget::HideNotification()
{
	SetVisibility(ESlateVisibility::Collapsed);
}
