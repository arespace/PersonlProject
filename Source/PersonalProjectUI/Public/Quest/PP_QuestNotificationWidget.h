// 任务系统 — 浮动通知 Widget
// 任务接取/完成/目标更新时的浮动提示

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_QuestNotificationWidget.generated.h"

class UTextBlock;

/**
 * 任务浮动通知 Widget
 *
 * 职责：
 *   1. 显示短暂的浮动文本通知（如 "任务接取：初出茅庐"）
 *   2. 支持自定义显示时长
 *   3. 显示后自动隐藏（可播放动画）
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_QuestNotificationWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 显示一条浮动通知
	 * @param Message 通知文本
	 * @param Duration 显示时长（秒），默认3秒
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest|UI")
	void ShowNotification(FText Message, float Duration = 3.0f);

protected:
	virtual void NativeConstruct() override;

	// ===== 蓝图绑定控件 =====

	/** 通知文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NotificationText;

private:
	/** 自动隐藏计时器句柄 */
	FTimerHandle HideTimerHandle;

	/** 隐藏通知 */
	void HideNotification();
};
