// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/EditableTextBox.h"
#include "Components/ComboBoxString.h"
#include "Game/PP_SessionManager.h"
#include "PP_MainMenuWidget.generated.h"

class APP_LobbyHUD;

UCLASS()
class PERSONALPROJECT_API UPP_MainMenuWidget : public UUserWidget
{
	GENERATED_BODY()

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


private:
	// 辅助函数：安全获取 LobbyHUD
	APP_LobbyHUD* GetLobbyHUD() const;

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
};