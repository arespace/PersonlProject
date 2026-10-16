// Chen Tianbao uses GAS to create online battle games

#include "UI/Widgets/PP_NotificationWidget.h"

#include "Components/PanelWidget.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBoxSlot.h"
#include "TimerManager.h"

UPP_NotificationWidget::UPP_NotificationWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Notification;
}

void UPP_NotificationWidget::AddMessage(const FString& Message)
{
	// 通知蓝图（蓝图可覆写显示逻辑）
	BP_OnMessageAdded(Message);

	if (!MessageContainer)
	{
		return;
	}

	// 路径 A：配置了 NotificationEntryClass → 创建条目 Widget 并尝试设置文本
	if (NotificationEntryClass)
	{
		UUserWidget* NewEntry = CreateWidget<UUserWidget>(this, NotificationEntryClass);
		if (NewEntry)
		{
			// 尝试在条目 Widget 中找到名为 MessageText 的 TextBlock 并设置文字
			if (UTextBlock* MsgText = Cast<UTextBlock>(NewEntry->GetWidgetFromName(TEXT("MessageText"))))
			{
				MsgText->SetText(FText::FromString(Message));
			}

			MessageContainer->AddChild(NewEntry);
			ActiveMessages.Add(NewEntry);
		}
	}
	// 路径 B：未配置 NotificationEntryClass → 直接用 TextBlock 兜底渲染
	else
	{
		UTextBlock* MsgText = NewObject<UTextBlock>(this);
		if (MsgText)
		{
			MsgText->SetText(FText::FromString(Message));
			MsgText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
			MessageContainer->AddChild(MsgText);

			// 用 Map 记录 TextBlock → 创建时间，供后续定时清理
			TextBlockTimestamps.Add(MsgText, GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f);
		}
	}

	// 超出最大显示数时移除最旧的消息
	while (ActiveMessages.Num() > MaxVisibleMessages)
	{
		UUserWidget* Oldest = ActiveMessages[0];
		if (Oldest)
		{
			Oldest->RemoveFromParent();
		}
		ActiveMessages.RemoveAt(0);
	}

	// 定时清理旧消息（仅当使用 TextBlock 兜底路径时）
	if (!NotificationEntryClass && MessageDisplayTime > 0.f && GetWorld())
	{
		// 启动清理 Timer（如果还没启动）
		if (!CleanupTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().SetTimer(
				CleanupTimerHandle,
				this,
				&UPP_NotificationWidget::CleanupExpiredTextBlocks,
				0.5f,  // 每 0.5 秒检查一次
				true   // 循环
			);
		}
	}
}

void UPP_NotificationWidget::CleanupExpiredTextBlocks()
{
	if (!MessageContainer || !GetWorld()) return;

	const float Now = GetWorld()->GetTimeSeconds();
	TArray<UTextBlock*> ExpiredBlocks;

	for (auto& Pair : TextBlockTimestamps)
	{
		if (Pair.Key && (Now - Pair.Value >= MessageDisplayTime))
		{
			ExpiredBlocks.Add(Pair.Key);
		}
	}

	for (UTextBlock* Block : ExpiredBlocks)
	{
		Block->RemoveFromParent();
		TextBlockTimestamps.Remove(Block);
	}

	// 全部清理完毕则停止 Timer
	if (TextBlockTimestamps.IsEmpty())
	{
		GetWorld()->GetTimerManager().ClearTimer(CleanupTimerHandle);
	}
}
