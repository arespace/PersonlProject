// NPC对话系统 — 对话结束选项面板 UMG 基类
// 根据 EPP_DialogueEndAction 显示对应按钮，点击按钮通过 PlayerController 上报服务器

#pragma once

#include "CoreMinimal.h"
#include "Framework/PP_UIUserWidget.h"
#include "Data/PP_DialogueData.h"
#include "PP_DialogueChoiceWidget.generated.h"

class UButton;
class UTextBlock;
class APP_NPCActor;
class APlayerController;

/**
 * 对话结束选项面板 Widget 基类
 *
 * 职责：
 *   1. 由 UPP_DialogueWidget::ShowEndChoices 创建并 AddToViewport
 *   2. 按 EPP_DialogueEndAction 显示/隐藏对应按钮：
 *        - OfferQuest        → OfferQuestButton（Task5 接取任务，当前仅上报）
 *        - SubmitQuest       → SubmitQuestButton（Task5 提交任务，当前仅上报）
 *        - OpenShop          → OpenShopButton（服务端打开商店UI）
 *        - ChainNextDialogue → ContinueButton（链式跳到 NextDialogueID）
 *        - 任意状态均显示 CloseButton
 *   3. 点击任意功能按钮 → PlayerController::Server_ConfirmDialogueEndAction → 服务器统一处理
 *
 * 命名约束（关键）：
 *   - UWidget 基类含公有成员 `Slot`，本类按钮变量一律使用具名（OfferQuestButton/OpenShopButton 等）
 *   - 严禁出现名为 `Slot` 的变量、参数、UFUNCTION 形参
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_DialogueChoiceWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_DialogueChoiceWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/**
	 * 初始化选项面板，按动作类型设置按钮可见性
	 * @param InAction     当前对话的结束动作
	 * @param InOwningNPC  发起对话的 NPC（用于服务端 OpenShop 等动作定位组件）
	 * @param InDialogueID 当前对话 DialogueID（与 RelatedQuestID 配合上报服务器）
	 * @param InOwnerPC    本地玩家控制器
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void SetupChoices(EPP_DialogueEndAction InAction, APP_NPCActor* InOwningNPC, FName InDialogueID, APlayerController* InOwnerPC);

	/** 关闭选项面板（移除自身） */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void CloseChoices();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	// ===== 蓝图绑定按钮（具名，避免与 UWidget::Slot 冲突） =====

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> OfferQuestButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> SubmitQuestButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> OpenShopButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> ContinueButton;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** 可选的提示文字（如「请选择」） */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> PromptText;

	// ===== 状态 =====

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	EPP_DialogueEndAction CurrentAction = EPP_DialogueEndAction::None;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	FName CurrentDialogueID;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	TWeakObjectPtr<APP_NPCActor> OwningNPC;

	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	TWeakObjectPtr<APlayerController> OwnerPC;

private:
	/** 上报服务器并关闭本面板 */
	void ReportAndClose(EPP_DialogueEndAction Action);

	// ===== 按钮回调 =====

	UFUNCTION()
	void OnOfferQuestClicked();

	UFUNCTION()
	void OnSubmitQuestClicked();

	UFUNCTION()
	void OnOpenShopClicked();

	UFUNCTION()
	void OnContinueClicked();

	UFUNCTION()
	void OnCloseClicked();
};
