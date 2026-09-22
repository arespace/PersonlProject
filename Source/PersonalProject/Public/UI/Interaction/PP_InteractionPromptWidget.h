// 采集系统 — 交互提示Widget
// 显示「按E采集」等交互提示文本，玩家进入交互范围时显示，离开时隐藏

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_InteractionPromptWidget.generated.h"

class UTextBlock;

/**
 * 交互提示Widget
 *
 * 职责：
 *   1. 通过 UTextBlock 显示交互提示文本（如「按E采集」）
 *   2. 提供 SetPromptText / ShowWidget / HideWidget 接口
 *   3. 玩家进入采集物 InteractionRange 时由外部调用 ShowWidget 显示
 *
 * 蓝图使用：创建继承此类的UMG蓝图，放置名为 PromptText 的 TextBlock 控件
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API UPP_InteractionPromptWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 设置提示文本
	 * @param InText 提示内容（如「按E采集」）
	 */
	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void SetPromptText(const FText& InText);

	/** 显示提示Widget */
	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void ShowWidget();

	/** 隐藏提示Widget */
	UFUNCTION(BlueprintCallable, Category = "Interaction|UI")
	void HideWidget();

protected:
	//~ Begin UUserWidget Interface
	virtual void NativeConstruct() override;
	//~ End UUserWidget Interface

	/** 提示文本控件 — 蓝图中放置同名 TextBlock 控件 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	/** 默认提示文本（可在编辑器中配置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaction|UI")
	FText DefaultPromptText;
};
