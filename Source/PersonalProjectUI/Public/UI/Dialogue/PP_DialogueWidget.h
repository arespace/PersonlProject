// NPC对话系统 — 对话框 UMG 基类
// 显示当前台词，提供「下一句」按钮逐条推进，最后一行后切换到选项面板

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "Dialogue/Data/PP_DialogueData.h"
#include "PP_DialogueWidget.generated.h"

class UTextBlock;
class UButton;
class APP_NPCActor;
class APP_PlayerController;
class UPP_DialogueChoiceWidget;

/**
 * 单行台词推进事件
 * @param NewLineIndex 推进后的台词索引（0-based）
 * @param NewLineText  当前台词内容
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueLineAdvancedUI, int32, NewLineIndex, const FText&, NewLineText);

/**
 * 对话框 Widget 基类
 *
 * 职责：
 *   1. 由 PlayerController::Client_OpenDialogueUI 加载并 AddToViewport
 *   2. 通过 SetupDialogue 接收一段 FPP_DialogueData，初始化 Speaker / 第一行台词
 *   3. NextButton 点击触发 AdvanceLine 推进到下一行；越界时切换到 ChoiceWidget
 *   4. 维护 CurrentLineIndex 与 OwningNPC / DialogueID，传递给 ChoiceWidget 上报服务器
 *
 * UMG 绑定：
 *   - SpeakerNameText / LineText / NextButton 使用 BindWidgetOptional，允许蓝图自由布局
 *
 * 命名约束（关键）：
 *   - UWidget 基类已有公有成员 `Slot`，本类所有变量/函数参数严禁使用 `Slot`
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_DialogueWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_DialogueWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	/**
	 * 初始化一段对话
	 * @param InData      对话数据（按值拷贝）
	 * @param InOwningNPC 发起对话的 NPC（用于结束动作回传服务器）
	 * @param InOwnerPC   本地玩家控制器（用于 ChoiceWidget 上报 ServerRPC）
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void SetupDialogue(const FPP_DialogueData& InData, APP_NPCActor* InOwningNPC, APP_PlayerController* InOwnerPC);

	/** 推进到下一行台词；若已到最后一行则切换到结束选项面板 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void AdvanceLine();

	/** 切换到结束选项面板（移除自身 → 创建 ChoiceWidget → AddToViewport） */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void ShowEndChoices();

	/** 关闭对话（移除自身） */
	UFUNCTION(BlueprintCallable, Category = "Dialogue|UI")
	void CloseDialogue();

	/** 单行推进事件，蓝图UI可用作打字机动画起点 */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|UI|Events")
	FOnDialogueLineAdvancedUI OnLineAdvanced;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	// ===== 蓝图绑定控件（BindWidgetOptional，允许蓝图自由命名/缺省） =====

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SpeakerNameText;

	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> LineText;

	/** 「下一句 / 打开商店」按钮 — 普通 NPC 显示「继续」，商人 NPC 显示「打开商店」且点击走 OpenShop RPC */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> NextButton;

	/** 「关闭」按钮 — 始终可见，点击直接关闭对话 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UButton> CloseButton;

	/** 结束选项面板蓝图类（在蓝图子类中配置） */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	TSubclassOf<UPP_DialogueChoiceWidget> ChoiceWidgetClass;

	// ===== 状态 =====

	/** 当前段对话数据（按值持有，便于运行期推进） */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	FPP_DialogueData CurrentData;

	/** 当前台词索引（0-based） */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	int32 CurrentLineIndex = 0;

	/** 发起对话的 NPC（弱持有，避免循环引用） */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	TWeakObjectPtr<APP_NPCActor> OwningNPC;

	/** 本地玩家控制器（ChoiceWidget 上报服务器时使用） */
	UPROPERTY(BlueprintReadOnly, Category = "Dialogue|UI")
	TWeakObjectPtr<APP_PlayerController> OwnerPC;

private:
	/** 渲染当前 CurrentLineIndex 指向的台词到 LineText */
	void RenderCurrentLine();

	/** 刷新 NextButton 文字：商人 NPC 显示「打开商店」，普通 NPC 显示「继续」 */
	void RefreshButtonVisibility();

	/** NextButton 点击回调 — 商人→打开商店, 普通NPC→推进对话 */
	UFUNCTION()
	void OnNextButtonClicked();

	/** CloseButton 点击回调 — 直接关闭对话 */
	UFUNCTION()
	void OnCloseButtonClicked();
};
