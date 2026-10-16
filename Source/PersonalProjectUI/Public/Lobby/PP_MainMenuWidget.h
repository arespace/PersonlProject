// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Framework/PP_SessionManager.h"
#include "PP_MainMenuWidget.generated.h"

class APP_LobbyHUD;
class UTextBlock;

UCLASS()
class PERSONALPROJECTUI_API UPP_MainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 刷新房间列表（显示「正在搜索…」→ 调 FindSessions）
	 * 进入主菜单时由 HUD 自动调一次，避免房间列表初始为空；刷新按钮也走这里。
	 */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void RefreshRoomList();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> HostButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> JoinButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> SettingsButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> QuitButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UEditableTextBox> PlayerNameInput;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UEditableTextBox> IPAddressInput;

	/** 房间列表下拉框 - 蓝图中需添加名为 "RoomListDropdown" 的 ComboBoxString 控件 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UComboBoxString> RoomListDropdown;

	/** 刷新房间列表按钮 - 蓝图中需添加名为 "RefreshButton" 的 Button 控件 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> RefreshButton;

	/** 状态提示文本（搜索中 / 暂无房间 / 加入失败），蓝图里可选；有了它就不再往下拉框里塞状态项 */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UTextBlock> StatusText;

	/** 版本号文本（右下角小字），蓝图里可选；WBP_MainMenu 中需有名为 "VersionText" 的 TextBlock */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UTextBlock> VersionText;

	/** 版本号内容，可在 WBP_MainMenu 的 Class Defaults 里改 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby")
	FText VersionLabel = FText::FromString(TEXT("v0.1"));


private:
	// 辅助函数：安全获取 LobbyHUD
	APP_LobbyHUD* GetLobbyHUD() const;

	/** 读取昵称：优先输入框 → 上次保存的 → 兜底 "Player" */
	FString GetPlayerName() const;

	/** 记住昵称，下次进主菜单自动填入输入框 */
	void SavePlayerName(const FString& Name);

	/** 连接中状态：禁用主按钮并显示「正在连接…」，失败时恢复 */
	void SetConnectingState(bool bConnecting);

	UFUNCTION() void OnHostClicked();
	UFUNCTION() void OnJoinClicked();
	UFUNCTION() void OnSettingsClicked();
	UFUNCTION() void OnQuitClicked();

	/** 刷新按钮点击 - 调用 SessionManager->FindSessions() */
	UFUNCTION() void OnRefreshButtonClicked();


	/** 下拉框选项变化回调 */
	UFUNCTION() void OnRoomListSelectionChanged(FString SelectedItem, ESelectInfo::Type SelectionType);

	/** FindSessions 完成回调 - 更新下拉框内容 */
	UFUNCTION() void OnSessionsFound(const TArray<FRoomInfo>& Rooms);

	/** JoinSession 完成回调 */
	UFUNCTION() void OnJoinSessionComplete(bool bSuccess);

	/** 会话管理器实例 */
	UPROPERTY()
	TObjectPtr<UPP_SessionManager> SessionManager;

	/** 当前选中的房间索引（-1 表示未选择） */
	int32 SelectedSessionIndex = -1;

	/** 显示一行状态提示；bIsError=true 用红色且停 5s，否则灰色停 30s */
	void SetStatusMessage(const FString& Message, bool bIsError);

	UFUNCTION()
	void ClearStatusMessage();

	FTimerHandle StatusClearTimerHandle;
};