// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "PP_AttributeBarWidget.generated.h"

/**
 * UPP_AttributeBarWidget - 属性条Widget（血条/蓝条）
 * 屏幕空间版本的属性显示，替代原PP_AttributeWidget
 * 通过UpdateValue()更新属性值，蓝图实现视觉反馈
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_AttributeBarWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 更新属性值 */
	void UpdateValue(float NewValue, float NewMaxValue);

	/** 设置进度条填充颜色 */
	UFUNCTION(BlueprintCallable, Category = "PP|AttributeBar")
	void SetBarFillColor(FLinearColor Color);

	/** 设置数值文本颜色 */
	UFUNCTION(BlueprintCallable, Category = "PP|AttributeBar")
	void SetValueTextColor(FLinearColor Color);

	/** 获取当前值 */
	float GetValue() const { return CurrentValue; }

	/** 获取最大值 */
	float GetMaxValue() const { return CurrentMaxValue; }

	/** 获取属性百分比 (0~1) */
	float GetPercent() const;

protected:
	virtual void NativePreConstruct() override;

	/** 蓝图绑定 - 进度条 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UProgressBar> AttributeBar;

	/** 蓝图绑定 - 数值文本 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|AttributeBar")
	TObjectPtr<UTextBlock> ValueText;

	/** 进度条填充颜色（编辑器中可设默认值，运行时SetBarFillColor覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AttributeBar")
	FLinearColor BarFillColor = FLinearColor::White;

	/** 数值文本颜色（编辑器中可设默认值，运行时SetValueTextColor覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AttributeBar")
	FLinearColor ValueTextColor = FLinearColor::White;

	/** 蓝图实现事件 - 属性值变更时调用 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Value Updated"))
	void BP_OnValueUpdated(float NewValue, float NewMaxValue, float Percent);

private:
	float CurrentValue = 0.f;
	float CurrentMaxValue = 100.f;
};