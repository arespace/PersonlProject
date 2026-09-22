// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_CrosshairWidget.generated.h"

/**
 * UPP_CrosshairWidget - 准星组件
 * 显示屏幕中央准星，支持命中反馈动画
 * 简化实现，视觉表现通过蓝图配置
 */
UCLASS()
class PERSONALPROJECT_API UPP_CrosshairWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 触发命中反馈动画 */
	UFUNCTION(BlueprintCallable, Category = "PP|Crosshair")
	void PlayHitFeedback();

protected:

	/** 蓝图实现事件 - 命中反馈动画 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Hit Feedback"))
	void BP_OnHitFeedback();
};
