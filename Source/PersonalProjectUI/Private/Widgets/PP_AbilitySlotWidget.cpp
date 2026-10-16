// Chen Tianbao uses GAS to create online battle games

#include "Widgets/PP_AbilitySlotWidget.h"

#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void UPP_AbilitySlotWidget::InitializeSlot(const FGameplayTag& InAbilityTag)
{
	AbilityTag = InAbilityTag;
}

void UPP_AbilitySlotWidget::UpdateCooldown(float Remaining, float Duration)
{
	CooldownRemaining = Remaining;
	CooldownDuration = Duration;
	bIsOnCooldown = Remaining > 0.f;

	// 计算冷却百分比
	float Percent = 0.f;
	if (CooldownDuration > 0.f)
	{
		Percent = CooldownRemaining / CooldownDuration;
	}

	// 更新冷却文本
	if (CooldownText)
	{
		if (bIsOnCooldown)
		{
			CooldownText->SetText(FText::FromString(FString::Printf(TEXT("%.1f"), CooldownRemaining)));
			CooldownText->SetVisibility(ESlateVisibility::Visible);
		}
		else
		{
			CooldownText->SetVisibility(ESlateVisibility::Hidden);
		}
	}

	// 更新冷却遮罩透明度
	if (CooldownOverlay)
	{
		CooldownOverlay->SetVisibility(bIsOnCooldown ? ESlateVisibility::Visible : ESlateVisibility::Hidden);
	}

	// 通知蓝图更新动画
	BP_OnCooldownUpdated(Percent);
}
