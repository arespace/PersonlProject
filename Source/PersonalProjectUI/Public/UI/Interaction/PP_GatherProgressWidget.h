// 采集系统 — 采集进度条Widget
// 显示采集进度 0.0 → 1.0，采集开始时显示，完成/取消时隐藏

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_GatherProgressWidget.generated.h"

class UProgressBar;
class UTextBlock;

/**
 * 采集进度条Widget
 *
 * 职责：
 *   1. 通过 UProgressBar 展示采集进度（0.0 → 1.0）
 *   2. 提供 SetProgress / ShowWidget / HideWidget 方法供外部调用
 *   3. 采集开始时显示，完成/取消时自动隐藏
 *
 * 蓝图使用：创建继承此类的UMG蓝图，在Designer中放置名为 ProgressBar 的 ProgressBar 控件
 */
UCLASS(Blueprintable)
class PERSONALPROJECTUI_API UPP_GatherProgressWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 设置采集进度值
	 * @param InProgress 进度值（0.0 = 开始，1.0 = 完成）
	 */
	UFUNCTION(BlueprintCallable, Category = "Gather|UI")
	void SetProgress(float InProgress);

	/** 显示进度条Widget */
	UFUNCTION(BlueprintCallable, Category = "Gather|UI")
	void ShowWidget();

	/** 隐藏进度条Widget */
	UFUNCTION(BlueprintCallable, Category = "Gather|UI")
	void HideWidget();

	/** 设置采集物名称提示文本（可选） */
	UFUNCTION(BlueprintCallable, Category = "Gather|UI")
	void SetGatherItemName(const FText& InName);

protected:
	/** 进度条控件 — 蓝图中放置同名 ProgressBar 控件 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ProgressBar;

	/** 采集物名称文本（可选） — 蓝图中放置同名 TextBlock 控件 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GatherItemNameText;
};
