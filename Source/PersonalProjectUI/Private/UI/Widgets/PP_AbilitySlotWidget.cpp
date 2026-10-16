// Chen Tianbao uses GAS to create online battle games

#include "UI/Widgets/PP_AbilitySlotWidget.h"

#include "UI/PP_UIViewModel.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Components/SizeBox.h"

void UPP_AbilitySlotWidget::InitializeSlot(UPP_UIViewModel* InViewModel, const FGameplayTag& InAbilityTag)
{
	ViewModel = InViewModel;
	AbilityTag = InAbilityTag;

	// 绑定ViewModel冷却变更委托
	if (ViewModel)
	{
		ViewModel->OnAbilityCooldownChanged.AddDynamic(this, &UPP_AbilitySlotWidget::OnCooldownChanged);
	}
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

void UPP_AbilitySlotWidget::OnCooldownChanged(FGameplayTag Tag, float RemainingTime, float Duration)
{
	// 只响应自己绑定的能力Tag
	if (Tag.MatchesTagExact(AbilityTag))
	{
		UpdateCooldown(RemainingTime, Duration);

		if (RemainingTime <= 0.f)
		{
			BP_OnAbilityActivated();
		}
	}
}
