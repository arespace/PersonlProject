// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/PanelWidget.h"
#include "Components/ScrollBox.h"
#include "Components/EditableTextBox.h"
#include "Components/Image.h"
#include "Components/TextBlock.h"
#include "Framework/PP_LobbyTypes.h"
#include "Lobby/PP_ChatMessageWidget.h"
#include "PP_LobbyWidget.generated.h"

class APP_LobbyHUD;
class UTextBlock;
class UPP_PlayerEntryWidget;

UCLASS()
class PERSONALPROJECTUI_API UPP_LobbyWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void UpdatePlayerList(const TArray<FLobbyPlayerInfo>& Players);

	/** 添加聊天消息到聊天区域 */
	void AddChatMessage(const FString& PlayerName, const FString& Message);

	/** 添加系统消息到聊天区域（加入/创建房间等，整行高亮） */
	void AddSystemMessage(const FString& Message);

	void OnGameStarting(float Countdown);

	/** 房间设置同步（MaxPlayers / MapName / GameModeName 需要显示在大厅顶栏） */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void ApplyRoomSettings(const FRoomSettings& Settings);

	/** 房主迁移提示条显隐（房主离开时给用户一个明确反馈） */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby")
	void SetMigrationBannerVisible(bool bVisible);

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

	// ===== 新增：文本状态（蓝图里摆同名 TextBlock 即可，全部可选） =====

	/** 顶栏人数 "3 / 4" */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UTextBlock> PlayerCountText;

	/** 顶栏房间信息 "PPMap · Crash" */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UTextBlock> RoomMetaText;

	/** 底栏已准备统计 "已准备 2 / 3 · 等待 王五" */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UTextBlock> ReadyCountText;

	/** 开始按钮被禁用时的原因提示（替代 tooltip）"还有 1 人未准备" */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UTextBlock> StartBlockReasonText;

	/** 倒计时：底栏小字 "⏱ 05.0s" */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UTextBlock> CountdownChipText;

	/** 倒计时：中央大字 "5"（放在 CountdownOverlay 里） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UTextBlock> CountdownBigText;

	/** 倒计时：中央覆盖层容器（放大字 + "即将进入 XXX"） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UPanelWidget> CountdownOverlay;

	/** 房主迁移提示条（房主离开时显示，避免界面毫无反馈） */
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|State")
	TObjectPtr<UUserWidget> MigrationBanner;

	/**
	 * 房间内单个玩家条目 Widget（WBP_PlayerEntry）
	 *
	 * 必须在 WBP_Lobby 的 Class Defaults 里显式配置：这里是硬引用，cooker 会顺着 CDO 把它打进包。
	 * 不要再按字符串路径 LoadClass 兜底 —— 那种引用烘焙器看不见，打包后会加载失败并退回文本渲染。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby")
	TSubclassOf<UUserWidget> PlayerEntryClass;

	/**
	 * 聊天单条消息 Widget（WBP_ChatMessage）
	 *
	 * 必须在 WBP_Lobby 的 Class Defaults 里显式配置（硬引用，保证进包）。
	 * 未配置时会打 Error 日志并退回单行 TextBlock 写法（聊天会退化成旧样式，但不会丢消息）。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby")
	TSubclassOf<UPP_ChatMessageWidget> ChatMessageClass;

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

	/** 房主专属按钮：禁用 + 占位，不再用 Collapsed 隐藏（否则非房主时整排按钮会位移） */
	void ApplyHostOnlyButtons(bool bLocalIsHost);

	/** 刷新人数 / 已准备统计 / 开始按钮可用性 */
	void UpdateBottomBar(int32 ReadyCount, int32 TotalPlayers, bool bLocalIsHost);

	/** 聊天列表是否已滚到底（只有贴底时才自动跟随新消息，避免打断翻看历史） */
	bool IsChatScrolledToBottom() const;

	/** 倒计时每帧刷新（中央覆盖层 + 底栏小字） */
	void UpdateCountdown(float InDeltaTime);

	/** 未配置 WBP_PlayerEntry（或父类不对）时的兜底：退回文本渲染 */
	UTextBlock* CreateFallbackPlayerText(const FLobbyPlayerInfo& PlayerInfo);

	FTimerHandle LeaveTimerHandle;

	/** 缓存的最大人数，用于把空位补齐到 MaxPlayers */
	int32 CachedMaxPlayers = 4;

	/** 倒计时剩余秒数，< 0 表示当前不在倒计时 */
	float CountdownRemaining = -1.f;

	/** 上一次渲染的人数（ApplyRoomSettings 刷新人数分母时用） */
	int32 LastRenderedPlayerCount = 0;

	bool bIsDraggingPreview = false;
	FVector2D LastDragMousePosition;
	bool bPreviewImageReady = false;
};
