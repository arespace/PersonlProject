// Chen Tianbao uses GAS to create online battle games

#include "Widgets/PP_SettlementWidget.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "TimerManager.h"

UPP_SettlementWidget::UPP_SettlementWidget()
{
	OwningLayer = EPP_UILayer::Modal;
	bConsumesAllInput = false;
}

void UPP_SettlementWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// 绑定继续按钮
	if (ContinueButton)
	{
		ContinueButton->OnClicked.AddDynamic(this, &ThisClass::OnContinueClicked);
	}
}

void UPP_SettlementWidget::NativeDestruct()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoCloseTimerHandle);
	}

	Super::NativeDestruct();
}

FReply UPP_SettlementWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::R)
	{
		OnContinueClicked();
		return FReply::Handled();
	}
	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void UPP_SettlementWidget::ShowSettlement(const FPP_SettlementShowEvent& Event)
{
	// 填入数据
	if (BossNameText)
	{
		BossNameText->SetText(FText::FromName(Event.BossName));
	}
	if (TimeText)
	{
		const int32 TotalSeconds = FMath::FloorToInt(Event.TimeToKill);
		const int32 Minutes = TotalSeconds / 60;
		const int32 Seconds = TotalSeconds % 60;
		TimeText->SetText(FText::FromString(FString::Printf(TEXT("%02d:%02d"), Minutes, Seconds)));
	}
	if (GoldText)
	{
		GoldText->SetText(FText::FromString(FString::Printf(TEXT("+%d Gold"), Event.GoldReward)));
	}

	// 加到视口（Widget 可能已经在 Viewport 上，先移除再添加）
	if (!IsInViewport())
	{
		AddToViewport();
	}

	// 启动自动关闭计时
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(AutoCloseTimerHandle, this, &ThisClass::OnContinueClicked, AutoCloseTime, false);
	}
}

void UPP_SettlementWidget::OnContinueClicked()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(AutoCloseTimerHandle);
	}
	RemoveFromParent();
}
