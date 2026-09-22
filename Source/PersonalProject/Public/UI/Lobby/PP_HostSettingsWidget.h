// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Components/SpinBox.h"
#include "PP_HostSettingsWidget.generated.h"

class UPP_SessionManager;
class APP_LobbyHUD;

UCLASS()
class PERSONALPROJECT_API UPP_HostSettingsWidget : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UComboBoxString> MapSelector;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<USpinBox> MaxPlayersSpinBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UComboBoxString> GameModeSelector;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> ConfirmButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> BackButton;

private:
	// 辅助函数：安全获取 LobbyHUD
	APP_LobbyHUD* GetLobbyHUD() const;

	UFUNCTION() void OnConfirmClicked();
	UFUNCTION() void OnBackClicked();

	/** Session创建完成回调 */
	UFUNCTION() void OnCreateSessionCompleted(bool bSuccess);

	/** SessionManager实例（负责LAN会话注册） */
	UPROPERTY()
	TObjectPtr<UPP_SessionManager> SessionManager;

	/** 等待Session创建完成后执行的Travel URL */
	FString PendingTravelURL;
};
