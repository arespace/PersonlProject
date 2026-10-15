// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_TopBarWidget.generated.h"

class UButton;

/**
 * UPP_TopBarWidget - 顶部导航栏（通用组件，单类实现）
 *
 * 只做一件事：四个按钮，点了切到对应界面（直接调 APP_LobbyHUD 的 Show 函数）。
 * 没有页签子 Widget、没有数据驱动、没有选中态管理 —— 样式全在蓝图里摆。
 *
 * WBP_TopBar（Reparent 到本类）里摆四个按钮，命名必须一致（BindWidgetOptional，缺了不崩）：
 *   NavButton_MainMenu         「主菜单」
 *   NavButton_Lobby            「房间大厅」
 *   NavButton_CharacterSelect  「角色选择」
 *   NavButton_Settings         「设置」
 *
 * 四个界面（WBP_MainMenu / WBP_Lobby / WBP_CharacterSelect / WBP_Settings）
 * 顶部各放一个 WBP_TopBar 即可，宿主不需要写任何代码。
 */
UCLASS()
class PERSONALPROJECT_API UPP_TopBarWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Nav")
	TObjectPtr<UButton> NavButton_MainMenu;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Nav")
	TObjectPtr<UButton> NavButton_Lobby;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Nav")
	TObjectPtr<UButton> NavButton_CharacterSelect;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Nav")
	TObjectPtr<UButton> NavButton_Settings;

private:
	UFUNCTION() void OnMainMenuClicked();
	UFUNCTION() void OnLobbyClicked();
	UFUNCTION() void OnCharacterSelectClicked();
	UFUNCTION() void OnSettingsClicked();

	/** 获取当前大厅 HUD；不在大厅阶段（拿不到 HUD）时点击直接忽略 */
	class APP_LobbyHUD* GetLobbyHUD() const;
};
