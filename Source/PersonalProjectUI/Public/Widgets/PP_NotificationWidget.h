// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "Components/PanelWidget.h"
#include "PP_NotificationWidget.generated.h"

class UTextBlock;

/**
 * UPP_NotificationWidget - 通知消息Widget
 * 支持队列显示消息，自动淡出消失
 * 用于击杀通知、系统消息等
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_NotificationWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_NotificationWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	/** 添加一条通知消息 */
	UFUNCTION(BlueprintCallable, Category = "PP|Notification")
	void AddMessage(const FString& Message);

protected:

	/** 蓝图绑定 - 通知消息容器 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UPanelWidget> MessageContainer;

	/** 单条通知Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Notification")
	TSubclassOf<UUserWidget> NotificationEntryClass;

	/** 最大同时显示消息数 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Notification")
	int32 MaxVisibleMessages = 5;

	/** 消息显示时长（秒） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Notification")
	float MessageDisplayTime = 3.f;

	/** 蓝图实现事件 - 添加新消息 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Message Added"))
	void BP_OnMessageAdded(const FString& Message);

private:

	/** 当前消息列表（NotificationEntryClass 路径） */
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> ActiveMessages;

	/** TextBlock 兜底路径：记录每个 TextBlock 的创建时间，用于定时清理 */
	TMap<UTextBlock*, float> TextBlockTimestamps;

	/** 清理过期 TextBlock 的定时器句柄 */
	FTimerHandle CleanupTimerHandle;

	/** 定时移除超过 MessageDisplayTime 的兜底 TextBlock */
	void CleanupExpiredTextBlocks();
};
