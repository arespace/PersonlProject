// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Player/PP_LobbyPlayerState.h"
#include "Player/PP_UIGateway.h"
#include "PP_LobbyHUD.generated.h"

class UPP_MainMenuWidget;
class UPP_LobbyWidget;
class UPP_HostSettingsWidget;
class UPP_CharacterSelectWidget;
class UPP_RoomSettingsWidget;
class UPP_SettingsWidget;
class APP_CharacterPreviewActor;
class UTextureRenderTarget2D;

/**
 * APP_LobbyHUD - 大厅UI调度器
 * 管理大厅阶段所有Widget的创建/销毁/显示/隐藏
 * 类似战斗阶段PP_HUD的角色
 */
UCLASS()
class PERSONALPROJECTUI_API APP_LobbyHUD : public AHUD, public IPP_UIGateway
{
	GENERATED_BODY()

public:
	// ===== IPP_UIGateway 接口实现（实现在 PP_UICrossModule.cpp）=====
	virtual void PushChatMessage_Implementation(const FString& PlayerName, const FString& Message) override;
	virtual void HandlePlayerListUpdated_Implementation(const TArray<FLobbyPlayerInfo>& Players) override;
	virtual void HandleRoomSettingsUpdated_Implementation(const FRoomSettings& Settings) override;
	virtual void HandleGameStarting_Implementation(float Countdown) override;
	virtual void HandleHostMigrating_Implementation(const FHostMigrationData& MigrationData) override;

	APP_LobbyHUD();
	virtual void BeginPlay() override;

	// ===== 菜单控制 =====

	void ShowMainMenu();
	void HideMainMenu();

	/**
	 * 当前是否还在房间里
	 * 判断依据与 BeginPlay 一致：Standalone = 还没进房间（主菜单阶段），
	 * 非 Standalone（ListenServer / Client）= 正在房间内。
	 * 导航栏「主菜单」按钮用它决定能不能跳转。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Lobby|Flow")
	bool IsInRoom() const;

	void ShowLobby();
	void HideLobby();

	void ShowHostSettings();
	void HideHostSettings();

	void ShowCharacterSelect();
	void HideCharacterSelect();

	void ShowRoomSettings();
	void HideRoomSettings();

	void ShowSettings();
	void HideSettings();

	/** 更新预览角色模型 */
	void UpdatePreviewCharacter(FName CharacterId);

	/** 获取预览RenderTarget（供Widget UImage直接显示） */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby|Preview")
	UTextureRenderTarget2D* GetPreviewRenderTarget() const;

	/** 拖拽旋转预览角色 */
	void RotatePreviewCharacter(float YawDelta, float PitchDelta);

	// ===== 数据更新（由PlayerController RPC调用） =====

	/** 玩家列表更新回调 */
	void OnPlayerListUpdated(const TArray<FLobbyPlayerInfo>& Players);

	/** 房间设置更新回调 */
	void OnRoomSettingsUpdated(const FRoomSettings& Settings);

	/** 游戏即将开始回调 */
	void OnGameStarting(float Countdown);

	/** 房主迁移中：通知大厅显示迁移提示条 */
	void NotifyHostMigrating();

	/** 添加聊天消息到大厅聊天区域 */
	void AddChatMessage(const FString& PlayerName, const FString& Message);

	/** 添加系统消息到大厅聊天区域（加入/创建房间等提示） */
	void AddSystemMessage(const FString& Message);

	/** 设置待使用的玩家名（主菜单阶段保存，进入Lobby后发送给服务器） */
	void SetPendingPlayerName(const FString& Name) { PendingPlayerName = Name; }
	FString GetPendingPlayerName() const { return PendingPlayerName; }

protected:

	// ===== ZOrder分层常量 =====
	static constexpr int32 ZOrder_MainMenu = 10;
	static constexpr int32 ZOrder_Lobby = 20;
	static constexpr int32 ZOrder_CharacterSelect = 30;
	static constexpr int32 ZOrder_Settings = 40;

	// ===== Widget类配置 =====

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Widgets")
	TSubclassOf<UUserWidget> MainMenuWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Widgets")
	TSubclassOf<UUserWidget> LobbyWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Widgets")
	TSubclassOf<UUserWidget> HostSettingsWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Widgets")
	TSubclassOf<UUserWidget> CharacterSelectWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Widgets")
	TSubclassOf<UUserWidget> RoomSettingsWidgetClass;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Widgets")
	TSubclassOf<UUserWidget> SettingsWidgetClass;

private:

	/** 本地缓存的角色选择（Lobby前使用） */
	FName LocalSelectedCharacterId;

	/** 主菜单 */
	UPROPERTY()
	TObjectPtr<UPP_MainMenuWidget> MainMenuWidget;

	/** 大厅核心 */
	UPROPERTY()
	TObjectPtr<UPP_LobbyWidget> LobbyWidget;

	/** 创建房间 */
	UPROPERTY()
	TObjectPtr<UPP_HostSettingsWidget> HostSettingsWidget;

	/** 角色选择 */
	UPROPERTY()
	TObjectPtr<UPP_CharacterSelectWidget> CharacterSelectWidget;

	/** 房间设置 */
	UPROPERTY()
	TObjectPtr<UPP_RoomSettingsWidget> RoomSettingsWidget;

	/** 游戏设置 */
	UPROPERTY()
	TObjectPtr<UPP_SettingsWidget> SettingsWidget;

	/** 确保预览Actor存在并显示，切换摄像机到预览视角 */
	void ShowPreviewActor();

	/** 创建所有Widget */
	void CreateWidgets();

	/** 设置输入模式为UI */
	void SetUIInputMode();

	/** 设置输入模式为GameAndUI */
	void SetGameAndUIInputMode();

	/** 隐藏所有菜单 Widget */
	void HideAllMenus();

	/** 玩家昵称（主菜单输入后暂存） */
	FString PendingPlayerName;

	/** 预览Actor引用（在Lobby地图中预放或动态Spawn） */
	UPROPERTY()
	TObjectPtr<APP_CharacterPreviewActor> PreviewActor;

	/** 预览Actor的类（用于动态Spawn） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Preview")
	TSubclassOf<APP_CharacterPreviewActor> PreviewActorClass;

	/** 预览Actor的Spawn位置 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Preview")
	FVector PreviewSpawnLocation = FVector(0.f, 500.f, 0.f);

	/** 预览Actor的Spawn旋转 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Preview")
	FRotator PreviewSpawnRotation = FRotator::ZeroRotator;
};