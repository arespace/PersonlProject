// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "Components/Button.h"
#include "PP_PauseMenuWidget.generated.h"

/**
 * UPP_PauseMenuWidget - 暂停菜单Widget
 * 显示暂停菜单，包含继续游戏/设置/退出等按钮
 * 按钮点击事件通过蓝图绑定实现
 */
UCLASS()
class PERSONALPROJECT_API UPP_PauseMenuWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_PauseMenuWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	/** 继续游戏 - 隐藏暂停菜单 */
	UFUNCTION(BlueprintCallable, Category = "PP|PauseMenu")
	void ResumeGame();

protected:

	/** 蓝图绑定 - 继续游戏按钮 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ResumeButton;

	/** 蓝图绑定 - 退出游戏按钮 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> QuitButton;

	/** 蓝图绑定 - 返回登录界面按钮 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UButton> ReturnToLoginButton;

	/** 登录界面Widget类 - 在WBP_PauseMenu中指定WBP_LoginView */
	UPROPERTY(EditDefaultsOnly, Category = "PP|PauseMenu")
	TSubclassOf<UUserWidget> LoginWidgetClass;

	virtual void NativeOnInitialized() override;
	virtual void NativeConstruct() override;

private:

	/** 继续游戏按钮点击回调 */
	UFUNCTION()
	void OnResumeClicked();

	/** 退出游戏按钮点击回调 */
	UFUNCTION()
	void OnQuitClicked();

	/** 返回登录界面按钮点击回调 */
	UFUNCTION()
	void OnReturnToLoginClicked();
};
