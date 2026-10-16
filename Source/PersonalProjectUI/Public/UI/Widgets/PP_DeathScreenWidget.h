// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "Components/TextBlock.h"
#include "PP_DeathScreenWidget.generated.h"

/**
 * UPP_DeathScreenWidget - 死亡画面Widget
 * 显示死亡遮罩和复活倒计时
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_DeathScreenWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_DeathScreenWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	/** 显示复活倒计时 */
	void ShowRespawnCountdown(float RespawnTime);

protected:

	/** 蓝图绑定 - 倒计时文本 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> RespawnCountdownText;

	/** 蓝图绑定 - 死亡提示文本 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DeathMessageText;

	/** 蓝图实现事件 - 死亡画面显示 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Death Screen Shown"))
	void BP_OnDeathScreenShown(float RespawnTime);

	/** 蓝图实现事件 - 复活完成 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Respawn Complete"))
	void BP_OnRespawnComplete();

private:

	/** 倒计时更新定时器回调（0.5s 间隔，替代每帧 Tick） */
	void TickCountdown();

	/** 复活剩余时间 */
	float RespawnRemaining = 0.f;

	/** 倒计时更新定时器句柄 */
	FTimerHandle CountdownTimerHandle;
};
