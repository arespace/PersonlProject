// Chen Tianbao uses GAS to create online battle games

#include "UI/Widgets/PP_DamageNumberWidget.h"

#include "Components/TextBlock.h"

void UPP_DamageNumberWidget::Configure(float Damage, FVector WorldLocation, bool bIsHeal, bool bIsCritical)
{
	CachedWorldPosition = WorldLocation;
	DisplayTime = 0.f;
	bAnimationFinished = false;

	// 设置伤害文本
	if (DamageText)
	{
		const int32 DisplayDamage = FMath::RoundToInt(Damage);
		if (bIsHeal)
		{
			DamageText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), DisplayDamage)));
		}
		else
		{
			DamageText->SetText(FText::FromString(FString::Printf(TEXT("%d"), DisplayDamage)));
		}
	}

	// 通知蓝图播放动画
	BP_OnDamageConfigured(Damage, bIsHeal, bIsCritical);
}

void UPP_DamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (bAnimationFinished)
	{
		return;
	}

	DisplayTime += InDeltaTime;
	if (DisplayTime >= MaxDisplayTime)
	{
		bAnimationFinished = true;
		OnFinished.Broadcast(this);
		return;
	}

	// 将世界坐标投影到屏幕坐标
	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		return;
	}

	FVector2D ScreenPosition;
	if (PC->ProjectWorldLocationToScreen(CachedWorldPosition, ScreenPosition, true))
	{
		// 垂直飘动偏移（从时间直接推导，无需累积状态）
		ScreenPosition.Y -= DisplayTime * FloatSpeed;
		SetPositionInViewport(ScreenPosition);
	}
}
