// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Player/PP_LobbyPlayerState.h"
#include "PP_LobbyWidget.generated.h"

class APP_LobbyHUD;

UCLASS()
class PERSONALPROJECT_API UPP_LobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void UpdatePlayerList(const TArray<FLobbyPlayerInfo>& Players);

	/** 添加聊天消息到聊天区域 */
	void AddChatMessage(const FString& PlayerName, const FString& Message);

	void OnGameStarting(float Countdown);

protected:
	virtual void NativeOnInitialized() override;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UPanelWidget> PlayerListContainer;
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UScrollBox> ChatDisplayArea;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UEditableTextBox> ChatInputBox;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> ReadyButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> StartButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> CharacterSelectButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> RoomSettingsButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UButton> LeaveButton;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby")
	TObjectPtr<UImage> CharacterPreviewImage;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby")
	TSubclassOf<UUserWidget> PlayerEntryClass;

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Player List Updated"))
	void BP_OnPlayerListUpdated(const TArray<FLobbyPlayerInfo>& Players);

	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Game Starting"))
	void BP_OnGameStarting(float Countdown);

	// Preview rotation via mouse drag
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

private:
	// 辅助函数：安全获取 LobbyHUD
	APP_LobbyHUD* GetLobbyHUD() const;

	UFUNCTION() void OnReadyClicked();
	UFUNCTION() void OnStartClicked();
	UFUNCTION() void OnCharacterSelectClicked();
	UFUNCTION() void OnRoomSettingsClicked();
	UFUNCTION() void OnLeaveClicked();
	UFUNCTION() void OnChatTextCommitted(const FText& Text, ETextCommit::Type CommitMethod);

	/** Timer 到期后执行 Disconnect（不依赖 Session 销毁回调） */
	void ExecuteDisconnect();

	FTimerHandle LeaveTimerHandle;

	bool bIsDraggingPreview = false;
	FVector2D LastDragMousePosition;
	bool bPreviewImageReady = false;
};
