// Chen Tianbao uses GAS to create online battle games

#include "UI/PP_AttributeBarWidget.h"

#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"

void UPP_AttributeBarWidget::NativePreConstruct()
{
	Super::NativePreConstruct();

	// 编辑器预览时应用默认颜色
	SetBarFillColor(BarFillColor);
	SetValueTextColor(ValueTextColor);
}

void UPP_AttributeBarWidget::UpdateValue(float NewValue, float NewMaxValue)
{
	CurrentValue = NewValue;
	CurrentMaxValue = FMath::Max(NewMaxValue, 1.f); // 避免除以0

	const float Percent = GetPercent();

	if (AttributeBar)
	{
		AttributeBar->SetPercent(Percent);
	}

	if (ValueText)
	{
		ValueText->SetText(FText::FromString(FString::Printf(TEXT("%.0f / %.0f"), CurrentValue, CurrentMaxValue)));
	}

	BP_OnValueUpdated(CurrentValue, CurrentMaxValue, Percent);
}

void UPP_AttributeBarWidget::SetBarFillColor(FLinearColor Color)
{
	BarFillColor = Color;
	if (AttributeBar)
	{
		AttributeBar->SetFillColorAndOpacity(Color);
	}
}

void UPP_AttributeBarWidget::SetValueTextColor(FLinearColor Color)
{
	ValueTextColor = Color;
	if (ValueText)
	{
		ValueText->SetColorAndOpacity(FSlateColor(Color));
	}
}

void UPP_AttributeBarWidget::SetValueText(FText Text)
{
	if (ValueText)
	{
		ValueText->SetText(Text);
	}
}

float UPP_AttributeBarWidget::GetPercent() const
{
	if (CurrentMaxValue <= 0.f)
	{
		return 0.f;
	}
	return FMath::Clamp(CurrentValue / CurrentMaxValue, 0.f, 1.f);
}