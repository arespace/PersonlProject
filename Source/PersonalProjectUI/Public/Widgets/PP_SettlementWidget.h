// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "PP_SettlementWidget.generated.h"

class UTextBlock;
class UButton;

/**
 * Boss 结算界面。
 * 由 HUD 收到 FPP_SettlementShowEvent 后创建并调用 ShowSettlement，
 * 3 秒后或按 R 淡出 RemoveFromParent。
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_SettlementWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_SettlementWidget();

	/** 填入结算数据并加入视口（由 HUD 的 EventBus 回调驱动） */
	void ShowSettlement(const struct FPP_SettlementShowEvent& Event);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** Boss 名称文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> BossNameText;

	/** 击杀用时文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TimeText;

	/** 金币奖励文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> GoldText;

	/** 继续按钮（点击触发 OnContinueClicked） */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> ContinueButton;

	/** 自动关闭计时（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Settlement")
	float AutoCloseTime{3.f};

private:
	/** 继续按钮绑定 */
	UFUNCTION()
	void OnContinueClicked();

	/** 自动关闭 Timer */
	FTimerHandle AutoCloseTimerHandle;
};
