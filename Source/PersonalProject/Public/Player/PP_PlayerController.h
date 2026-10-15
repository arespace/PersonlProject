// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "InputMappingContext.h"
#include "GameFramework/PlayerController.h"
#include "UI/Widgets/PP_ScoreboardWidget.h"
#include "Game/PP_SessionManager.h"
#include "Dialogue/Data/PP_DialogueData.h"
#include "PP_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UPP_UIViewModel;
class APP_LobbyHUD;
class APP_NPCActor;
class APP_GatherableActor;
class UPP_DialogueWidget;
struct FInputActionValue;
struct FGameplayTag;
struct FLobbyPlayerInfo;
struct FRoomSettings;
struct FHostMigrationData;

/** 对话推进事件（UI 侧切换台词时触发） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogueLineAdvanced, int32, NewLineIndex);

/** 对话结束事件（任意 EndAction 处理完成后触发，Task5 任务系统 UI 可挂钩） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueEnded, FName, DialogueID, FName, RelatedQuestID);

UCLASS()
class PERSONALPROJECT_API APP_PlayerController : public APlayerController
{
	GENERATED_BODY()
	
protected:
	virtual void SetupInputComponent() override;

public:
	// ===== 网络RPC =====

	/** 服务器设置玩家昵称 */
	UFUNCTION(Server, Reliable)
	void Server_SetPlayerName(const FString& NewName);

	/** 服务器发送聊天消息 */
	UFUNCTION(Server, Reliable)
	void Server_SendChatMessage(const FString& Message);

	/** 客户端接收聊天消息 */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveChatMessage(const FString& PlayerName, const FString& Message);

	/** 客户端接收通知消息 */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveNotification(const FString& Message);

	/** 客户端更新计分板数据 */
	UFUNCTION(Client, Reliable)
	void Client_UpdateScoreboard(const TArray<FScoreboardEntry>& Entries);

	// ===== 跨地图角色选择桥接 =====

	/** 存储从大厅带到战斗地图的角色选择（跨 ServerTravel 持久化，因为 PlayerController 不销毁） */
	void SetPendingCharacterId(FName CharacterId) { PendingCharacterId = CharacterId; }
	FName GetPendingCharacterId() const { return PendingCharacterId; }

	// ===== 大厅阶段 Server RPC =====

	/** 设置准备状态 */
	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bReady);

	/** 选择角色 */
	UFUNCTION(Server, Reliable)
	void Server_SelectCharacter(FName CharacterId);

	/** 更新房间设置（仅主机） */
	UFUNCTION(Server, Reliable)
	void Server_UpdateRoomSettings(const FRoomSettings& Settings);

	/** 请求开始游戏（仅主机） */
	UFUNCTION(Server, Reliable)
	void Server_StartGame();

	/** 踢出玩家（仅主机） */
	UFUNCTION(Server, Reliable)
	void Server_KickPlayer(int32 PlayerId);

	/** 房主请求离开并触发迁移 */
	UFUNCTION(Server, Reliable)
	void Server_RequestHostMigration();

	// ===== 大厅阶段 Client RPC =====

	/** 客户端接收玩家列表更新 */
	UFUNCTION(Client, Reliable)
	void Client_OnLobbyPlayerListUpdated(const TArray<FLobbyPlayerInfo>& Players);

	/** 客户端接收房间设置更新 */
	UFUNCTION(Client, Reliable)
	void Client_OnLobbySettingsUpdated(const FRoomSettings& Settings);

	/** 客户端接收游戏即将开始倒计时 */
	UFUNCTION(Client, Reliable)
	void Client_OnGameStartingCountdown(float Countdown);

	/** 客户端准备成为新房主（迁移数据） */
	UFUNCTION(Client, Reliable)
	void Client_PrepareHostMigration(const FHostMigrationData& MigrationData);

	/** 客户端收到房主迁移通知 */
	UFUNCTION(Client, Reliable)
	void Client_NotifyHostMigrating(const FHostMigrationData& MigrationData);

	// ===== NPC对话系统 RPC =====

	/**
	 * 客户端请求与 NPC 启动对话
	 * 服务器验证距离 → 调用 NPC->StartDialogue（StartDialogue 内部再发 Client_OpenDialogueUI）
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Dialogue")
	void Server_RequestStartDialogue(APP_NPCActor* NPC);

	/**
	 * 客户端打开对话 UI
	 * 加载 DialogueWidgetClass → AddToViewport → 从 NPC 的 DialogueTable 查询行 → SetupDialogue
	 */
	UFUNCTION(Client, Reliable, Category = "Dialogue")
	void Client_OpenDialogueUI(APP_NPCActor* NPC, FName DialogueID);

	/**
	 * 客户端提交对话结束动作（OpenShop / OfferQuest / SubmitQuest / ChainNextDialogue / None）
	 * 服务器验证后执行：
	 *   - OpenShop          → 通过 Client RPC 让客户端打开商店 UI
	 *   - OfferQuest        → TODO(Task5) 调用任务系统接取任务
	 *   - SubmitQuest       → TODO(Task5) 调用任务系统提交任务
	 *   - ChainNextDialogue → 服务端 → Client_OpenDialogueUI 跳到下一段
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Dialogue")
	void Server_ConfirmDialogueEndAction(APP_NPCActor* NPC, FName DialogueID, EPP_DialogueEndAction Action);

	/**
	 * 客户端打开商店 UI（Server_ConfirmDialogueEndAction 处理 OpenShop 时回调）
	 * 在本地加载 ShopWidgetClass → AddToViewport → InitializeShop
	 */
	UFUNCTION(Client, Reliable, Category = "Dialogue")
	void Client_OpenShopUI(APP_NPCActor* NPC);

	/**
	 * 客户端显示任务系统提示（通知栏）
	 * 用于服务器侧任务操作失败时反馈给玩家（如提交时目标未完成），避免静默无反馈
	 */
	UFUNCTION(Client, Reliable, Category = "Quest")
	void Client_NotifyQuestMessage(const FString& Message);

	// ===== 对话事件委托（供蓝图 UI 监听） =====

	/** 对话推进到新一行台词时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueLineAdvanced OnDialogueLineAdvanced;

	/** 对话结束时广播（DialogueID + RelatedQuestID 供 Task5 钩入） */
	UPROPERTY(BlueprintAssignable, Category = "Dialogue|Events")
	FOnDialogueEnded OnDialogueEnded;

	/**
	 * 交互按键入口：从相机前方做 trace 寻找 APP_NPCActor，若命中则调用其 Interact
	 * 由蓝图侧的 IA_Interact 输入动作绑定到本函数（C++ 不直接绑 IA 资产）
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Interact();

	// ===== 对话 UI 配置 =====

	/** 对话框 Widget 蓝图类（在 PC 蓝图子类中指定） */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	TSubclassOf<UPP_DialogueWidget> DialogueWidgetClass;

	/** 商店 Widget 蓝图类（OpenShop 动作回调时使用） */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	TSubclassOf<class UPP_ShopWidget> ShopWidgetClass;

	/** 交互范围（单位：cm），服务端 Server_RequestStartDialogue 距离验证使用 */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (ClampMin = "0.0"))
	float InteractMaxDistance = 350.f;

	/** 交互 trace 最大距离（cm），用于 Interact 时从相机前方做 trace */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue", meta = (ClampMin = "0.0"))
	float InteractTraceDistance = 500.f;

	// ===== 采集交互 RPC =====

	/** 客户端请求与采集物交互 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Gather")
	void Server_InteractGather(APP_GatherableActor* Target);

private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	/** 大厅选择的角色ID，跨 ServerTravel 携带到战斗地图 */
	FName PendingCharacterId;
	
	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> SecondaryAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> TertiaryAction;

	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> FourthAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "Crash|Input|Abilities")
	TObjectPtr<UInputAction> FifthAction;
	
	// ===== UI输入动作 =====

	/** 暂停/恢复菜单输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|UI")
	TObjectPtr<UInputAction> PauseAction;

	/** 计分板显示输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|UI")
	TObjectPtr<UInputAction> ScoreboardAction;

	/** 主界面开关输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|UI")
	TObjectPtr<UInputAction> HUDToggleAction;

	/** 对话/交互输入动作（E键触发NPC对话） */
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|UI")
	TObjectPtr<UInputAction> DialogueAction;

	/** 任务日志开关输入动作（J/Tab键打开任务面板） */
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|UI")
	TObjectPtr<UInputAction> QuestAction;

	/** 背包开关输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Input|UI")
	TObjectPtr<UInputAction> InventoryAction;

	// ===== 原有功能 =====
	void Jump();
	void StopJumping();
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void Primary();
	void Secondary();
	void Tertiary();
	void Fourth();
	void Fifth();
	void ActivateAbility(const FGameplayTag& AbilityTag) const;
	bool IsAlive() const;

	// ===== UI功能 =====
	void TogglePauseMenu();
	void ShowScoreboard();
	void HideScoreboard();
	void ToggleHUD();
	void ToggleQuestLog();
	void ToggleInventory();

	// ===== 大厅辅助 =====

	/** 获取LobbyHUD（大厅阶段使用） */
	APP_LobbyHUD* GetLobbyHUD() const;
};
