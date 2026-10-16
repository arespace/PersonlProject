// Chen Tianbao uses GAS to create online battle games

#include "Widgets/PP_DeathScreenWidget.h"

#include "Components/TextBlock.h"
#include "TimerManager.h"

UPP_DeathScreenWidget::UPP_DeathScreenWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Modal;
	bConsumesAllInput = true;
}

void UPP_DeathScreenWidget::ShowRespawnCountdown(float RespawnTime)
{
	RespawnRemaining = RespawnTime;

	// 更新显示
	if (RespawnCountdownText)
	{
		const int32 Seconds = FMath::CeilToInt(RespawnRemaining);
		RespawnCountdownText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Seconds)));
	}

	// 启动 0.5s 间隔定时器替代每帧 Tick
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(CountdownTimerHandle, this,
			&UPP_DeathScreenWidget::TickCountdown, 0.5f, true);
	}

	// 通知蓝图
	BP_OnDeathScreenShown(RespawnTime);
}

void UPP_DeathScreenWidget::TickCountdown()
{
	RespawnRemaining -= 0.5f;

	if (RespawnRemaining <= 0.f)
	{
		RespawnRemaining = 0.f;

		// 停止定时器
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().ClearTimer(CountdownTimerHandle);
		}

		if (RespawnCountdownText)
		{
			RespawnCountdownText->SetText(FText::FromString(TEXT("0")));
		}

		BP_OnRespawnComplete();
		return;
	}

	// 更新倒计时显示
	if (RespawnCountdownText)
	{
		const int32 Seconds = FMath::CeilToInt(RespawnRemaining);
		RespawnCountdownText->SetText(FText::FromString(FString::Printf(TEXT("%d"), Seconds)));
	}
}
