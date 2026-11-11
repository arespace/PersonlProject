
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Session/PP_LobbyActions.h"
#include "Dialogue/PP_DialogueRequester.h"
#include "Player/PP_UICombatGateway.h"
#include "Player/PP_UILobbyGateway.h"
#include "Player/PP_UIMessagingGateway.h"
#include "PP_PlayerController.generated.h"

class UInputMappingContext;
class UInputAction;
class UUserWidget;
class APP_NPCActor;
class APP_GatherableActor;
class APP_BonfireActor;
struct FInputActionValue;
struct FGameplayTag;
struct FLobbyPlayerInfo;
struct FRoomSettings;
struct FHostMigrationData;

/** 输入上下文：唯一决定当前 IMC 组合、Slate 输入模式与鼠标光标状态 */
UENUM(BlueprintType)
enum class EPP_InputContext : uint8
{
	/** 正常战斗：ALS + 项目玩法 IMC 全开，GameOnly，隐藏鼠标 */
	Gameplay,

	/** 背包/商店/任务等覆盖层：保持当前 IMC 组合，GameAndUI，显示鼠标 */
	Overlay,

	/** 暂停/对话等模态界面：移除项目 IMC，UIOnly，显示鼠标 */
	Modal,

	/** 大厅/登录：移除项目 IMC，UIOnly，显示鼠标 */
	FrontEnd
};

UCLASS()
class PERSONALPROJECT_API APP_PlayerController : public APlayerController, public IPP_DialogueRequester, public IPP_LobbyActions
{
	GENERATED_BODY()

public:
	// ===== IPP_DialogueRequester 接口实现（实现体在本类 PP_PlayerController.cpp，转发到自身 RPC）=====
	virtual void RequestStartDialogue_Implementation(APP_NPCActor* NPC) override;
	virtual void OpenDialogueUI_Implementation(APP_NPCActor* NPC, FName DialogueID) override;
	virtual void ConfirmDialogueEndAction_Implementation(UObject* NPC, FName DialogueID, EPP_DialogueEndAction Action) override;

	// ===== IPP_LobbyActions 接口实现（UI 大厅 Widget 经 Execute_ 调用，转发到自身 Server RPC）=====
	virtual void RequestSetReady_Implementation(bool bReady) override;
	virtual void RequestStartGame_Implementation() override;
	virtual void RequestHostMigration_Implementation() override;
	virtual void RequestSendChatMessage_Implementation(const FString& Message) override;
	virtual void RequestUpdateRoomSettings_Implementation(const FRoomSettings& Settings) override;
	virtual void RequestSelectCharacter_Implementation(FName CharacterId) override;

protected:
	virtual void SetupInputComponent() override;

public:
	/**
	 * 应用输入上下文：唯一的 IMC / 输入模式 / 鼠标光标切换入口。
	 * 每次切换会 FlushPressedKeys，避免按键残留与“关闭 UI 后立刻重复触发”。
	 */
	UFUNCTION(BlueprintCallable, Category = "PP|Input")
	void ApplyInputContext(EPP_InputContext NewContext);

	EPP_InputContext GetInputContext() const { return CurrentInputContext; }

	/** 新 Pawn 接管时按当前上下文同步角色移动 IMC（重生、换人都会走这里） */
	virtual void SetPawn(APawn* InPawn) override;

	// ===== 网络RPC =====

	/** 客户端请求在篝火休息（篝火 Actor 无 owning connection，Server RPC 必须由 PC 发） */
	UFUNCTION(Server, Reliable)
	void ServerInteractBonfire(APP_BonfireActor* Bonfire);

	// ===== 存档调试入口（控制台） =====

	/** 保存当前玩家进度：PP_SaveGame（客户端窗口执行会自动转发到服务器） */
	UFUNCTION(Exec)
	void PP_SaveGame();

	/** 读取存档并应用到当前玩家：PP_LoadGame（客户端窗口执行会自动转发到服务器） */
	UFUNCTION(Exec)
	void PP_LoadGame();

	/** 服务器发送聊天消息 */
	UFUNCTION(Server, Reliable)
	void Server_SendChatMessage(const FString& Message);

	/** 客户端接收聊天消息 */
	UFUNCTION(Client, Reliable)
	void Client_ReceiveChatMessage(const FString& PlayerName, const FString& Message);

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
	 *   - OfferQuest        → 从 NPC 对话表取 RelatedQuestID，调用 QuestComponent::AcceptQuest
	 *   - SubmitQuest       → 从 NPC 对话表取 RelatedQuestID，调用 QuestComponent::SubmitQuest（失败时通知客户端）
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

	/** 通用通知：服务器向玩家推送一条 HUD 通知（篝火休息、拾回魂等） */
	UFUNCTION(Client, Reliable, BlueprintCallable, Category = "PP|UI")
	void Client_NotifyMessage(const FString& Message);

	/**
	 * 交互按键入口：从相机前方做 trace 寻找 APP_NPCActor，若命中则调用其 Interact
	 * 由蓝图侧的 IA_Interact 输入动作绑定到本函数（C++ 不直接绑 IA 资产）
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void Interact();

	/**
	 * 篝火休息入口：与 Interact 相同的相机 trace，但只对篝火生效
	 * 绑定在 BonfireInteractAction（IA_BonfireInteract，默认数字键 8），避免与对话推进键 E 冲突
	 */
	UFUNCTION(BlueprintCallable, Category = "Dialogue")
	void InteractBonfire();

	// ===== 对话 UI 配置 =====

	/** 对话框 Widget 蓝图类（在 PC 蓝图子类中指定；类型用 UUserWidget 基类避免主玩法模块反向依赖 UI 模块，UI 模块内 Cast 使用） */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	TSubclassOf<UUserWidget> DialogueWidgetClass;

	/** 商店 Widget 蓝图类（OpenShop 动作回调时使用；类型说明同上） */
	UPROPERTY(EditDefaultsOnly, Category = "Dialogue|UI")
	TSubclassOf<UUserWidget> ShopWidgetClass;

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
	
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input")
	TArray<TObjectPtr<UInputMappingContext>> InputMappingContexts;

	/** UI 上下文（IMC_UIInterface）：Modal / FrontEnd 只保留这一个 IMC，保证关闭键仍能被 Enhanced Input 派发 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input")
	TSoftObjectPtr<UInputMappingContext> UIInputContext;

	/** 当前输入上下文 */
	EPP_InputContext CurrentInputContext = EPP_InputContext::Gameplay;

	/** 是否已至少应用过一次上下文（首次调用即使值相同也要执行清理与模式设置） */
	bool bInputContextInitialized = false;

	/** 已加入 LocalPlayer 子系统、由本类负责增删的项目 IMC */
	UPROPERTY(Transient)
	TArray<TObjectPtr<UInputMappingContext>> ActiveProjectInputContexts;

	/** 存档调试 RPC：Exec 在客户端窗口执行时转发到服务器 */
	UFUNCTION(Server, Reliable)
	void ServerPP_SaveGame();

	UFUNCTION(Server, Reliable)
	void ServerPP_LoadGame();

	/** 按上下文重算项目 IMC：Gameplay/Overlay 全开（priority=1），Modal/FrontEnd 仅 UI（priority=2） */
	void ApplyProjectInputContexts(EPP_InputContext Context);

	/** 解析 UI 上下文，蓝图为空时回退到默认资产路径 */
	UInputMappingContext* ResolveUIInputContext() const;

	/** 关闭最上层 Modal（IA_UICancel → UI 网关） */
	void CloseTopModal();

	/** 大厅选择的角色ID，跨 ServerTravel 携带到战斗地图 */
	FName PendingCharacterId;
	
	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Movement")
	TObjectPtr<UInputAction> JumpAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Movement")
	TObjectPtr<UInputAction> MoveAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Movement")
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Abilities")
	TObjectPtr<UInputAction> PrimaryAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Abilities")
	TObjectPtr<UInputAction> SecondaryAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Abilities")
	TObjectPtr<UInputAction> TertiaryAction;

	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Abilities")
	TObjectPtr<UInputAction> FourthAction;
	
	UPROPERTY(EditDefaultsOnly,Category = "PP|Input|Abilities")
	TObjectPtr<UInputAction> FifthAction;
	
	// ===== UI输入动作 =====

	/** 暂停/恢复菜单输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
	TObjectPtr<UInputAction> PauseAction;

	/** 统一返回/关闭输入动作（IA_UICancel，默认映射 Escape；关闭最上层 Modal） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
	TObjectPtr<UInputAction> UICancelAction;

	/** 计分板输入动作（IA_Scoreboard，默认映射数字键 7；按住显示、松开隐藏） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
	TObjectPtr<UInputAction> ScoreboardAction;

	/** 主界面开关输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
	TObjectPtr<UInputAction> HUDToggleAction;

	/** 对话/交互输入动作（E键触发NPC对话） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
	TObjectPtr<UInputAction> DialogueAction;

	/** 篝火休息输入动作（IA_BonfireInteract，默认映射数字键 8；交互键 E 与对话推进冲突，篝火改用独立键） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|Dialogue")
	TObjectPtr<UInputAction> BonfireInteractAction;

	/** 任务日志开关输入动作（J/Tab键打开任务面板） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
	TObjectPtr<UInputAction> QuestAction;

	/** 背包开关输入动作 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|UI")
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

	// ===== UI 网关转发辅助 =====

	/**
	 * 按上下文转发到 UI 网关接口，TInterface 取 IPP_UICombatGateway / IPP_UILobbyGateway / IPP_UIMessagingGateway
	 * （HUD 侧实现：APP_HUD / APP_LobbyHUD）。
	 *
	 * 用法：
	 *   ForwardToUI<IPP_UICombatGateway>(&IPP_UICombatGateway::Execute_RequestTogglePauseMenu);
	 *   ForwardToUI<IPP_UIMessagingGateway>(&IPP_UIMessagingGateway::Execute_PushChatMessage, PlayerName, Message);
	 *
	 * 说明：接口方法均为 BlueprintNativeEvent，C++ 侧只能经 UHT 静态分发器 Execute_Xxx 调用
	 *      （经接口指针直接调用会命中 check(0) 桩）；Execute_Xxx 内部要求目标已实现接口，
	 *      故此处保留 Cast 守卫。
	 */
	template <typename TInterface, typename TExecuteFunc, typename... TArgs>
	void ForwardToUI(TExecuteFunc ExecuteFunc, TArgs&&... Args)
	{
		if (Cast<TInterface>(GetHUD()) != nullptr)
		{
			ExecuteFunc(GetHUD(), Forward<TArgs>(Args)...);
		}
	}

	// ===== 大厅辅助 =====

	/**
	 * GetLobbyHUD 已移除（模块化拆分：主玩法模块不得 include UI 类型）。
	 * UI 侧需要 LobbyHUD 时用 Cast<APP_LobbyHUD>(GetOwningPlayerController()->GetHUD())。
	 */
};
