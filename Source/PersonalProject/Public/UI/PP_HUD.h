// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "PP_HUD.generated.h"

class UPP_UIViewModel;
class UPP_UIUserWidget;
class UPP_HUDWidget;
class UPP_DamageNumberWidget;
class UPP_ScoreboardWidget;
class UPP_PauseMenuWidget;
class UPP_DeathScreenWidget;
class UPP_NotificationWidget;
class UPP_WidgetPool;
class UPP_QuestListWidget;
class UPP_QuestTrackerWidget;
class UPP_InventoryWidget;

/**
 * APP_HUD - UI总调度器
 * 管理所有屏幕空间UI的生命周期与ZOrder层级
 * 职责：Widget创建/销毁/分层、ViewModel初始化、伤害数字生成
 */
UCLASS()
class PERSONALPROJECT_API APP_HUD : public AHUD
{
	GENERATED_BODY()

public:
	APP_HUD();
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ===== 菜单控制 =====

	/** 显示暂停菜单 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ShowPauseMenu();

	/** 隐藏暂停菜单 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void HidePauseMenu();

	/** 显示计分板 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ShowScoreboard();

	/** 隐藏计分板 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void HideScoreboard();

	/** 显示死亡画面 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ShowDeathScreen(float RespawnTime);

	/** 隐藏死亡画面 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void HideDeathScreen();

	/** 切换主界面HUD显示/隐藏 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ToggleMainHUD();

	/** 显示主界面HUD */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ShowMainHUD();

	/** 隐藏主界面HUD */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void HideMainHUD();

	/** 切换任务日志面板显示/隐藏 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ToggleQuestLog();

	/** 切换背包面板显示/隐藏 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void ToggleInventory();

	// ===== 伤害数字 =====

	/** 生成伤害数字 - 由ViewModel委托调用 */
	void SpawnDamageNumber(float Damage, FVector WorldLocation, bool bIsHeal, bool bIsCritical);

	// ===== 通知 =====

	/** 添加通知消息 */
	UFUNCTION(BlueprintCallable, Category = "PP|HUD")
	void AddNotification(const FString& Message);

	// ===== 获取器 =====

	/** 获取ViewModel */
	UPP_UIViewModel* GetViewModel() const { return ViewModel; }

	/** 获取计分板Widget（供PlayerController RPC调用） */
	UPP_ScoreboardWidget* GetScoreboardWidget() const { return ScoreboardWidget; }

	/** 暂停菜单是否可见 */
	bool IsPauseMenuVisible() const;

protected:

	// ===== Widget类配置 =====

	/** 战斗HUD Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** 伤害数字Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> DamageNumberWidgetClass;

	/** 计分板Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> ScoreboardWidgetClass;

	/** 暂停菜单Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> PauseMenuWidgetClass;

	/** 死亡画面Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> DeathScreenWidgetClass;

	/** 通知Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> NotificationWidgetClass;

	/** 任务日志面板Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> QuestListWidgetClass;

	/** 任务追踪器Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> QuestTrackerWidgetClass;

	/** 背包 Widget 类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Widgets")
	TSubclassOf<UUserWidget> InventoryWidgetClass;

	// ===== 伤害数字对象池配置 =====

	/** 伤害数字对象池初始大小 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Pool")
	int32 DamageNumberPoolSize = 16;

private:

	/** 当前主界面HUD是否可见 */
	bool bIsHUDVisible = true;

	/** 数据中介层 */
	UPROPERTY()
	TObjectPtr<UPP_UIViewModel> ViewModel;

	/** 战斗HUD根节点 */
	UPROPERTY()
	TObjectPtr<UPP_HUDWidget> HUDWidget;

	/** 计分板 */
	UPROPERTY()
	TObjectPtr<UPP_ScoreboardWidget> ScoreboardWidget;

	/** 暂停菜单 */
	UPROPERTY()
	TObjectPtr<UPP_PauseMenuWidget> PauseMenuWidget;

	/** 死亡画面 */
	UPROPERTY()
	TObjectPtr<UPP_DeathScreenWidget> DeathScreenWidget;

	/** 通知组件 */
	UPROPERTY()
	TObjectPtr<UPP_NotificationWidget> NotificationWidget;

	/** 伤害数字对象池 */
	UPROPERTY()
	TObjectPtr<UPP_WidgetPool> DamageNumberPool;

	/** 任务日志面板 */
	UPROPERTY()
	TObjectPtr<UPP_QuestListWidget> QuestListWidget;

	/** 任务追踪器 */
	UPROPERTY()
	TObjectPtr<UPP_QuestTrackerWidget> QuestTrackerWidget;

	/** 背包面板（缓存，不重复创建） */
	UPROPERTY()
	TObjectPtr<UPP_InventoryWidget> InventoryWidget;

	/** 初始化ViewModel */
	void InitializeViewModel();

	/**
	 * 以 Widget 自身 OwningLayer 作为 ZOrder 添加到视口
	 * 层级唯一真相来源是 UPP_UIUserWidget::OwningLayer（蓝图配置），HUD 不再维护独立 ZOrder 常量
	 */
	static void AddToViewportByLayer(UPP_UIUserWidget* Widget);

	/** 任务完成事件回调（EventBus 订阅 → 通知栏显示），任务系统不再直接调用 HUD */
	void OnQuestCompletedEvent(const struct FPP_QuestCompletedEvent& Event);

	/** 任务完成事件订阅句柄（EndPlay 时退订） */
	FDelegateHandle QuestCompletedEventHandle;

	/** 创建所有HUD Widget */
	void CreateHUDWidgets();

	/** 绑定ViewModel委托 */
	void BindViewModelDelegates();

	/** ViewModel伤害接收回调 */
	UFUNCTION()
	void OnDamageReceived(float Damage, FVector WorldLocation, bool bIsHeal, bool bIsCritical, AActor* SourceActor);

	/** ViewModel玩家死亡回调 */
	UFUNCTION()
	void OnPlayerDeath(AActor* DeadActor);

	/** ViewModel玩家复活回调 */
	UFUNCTION()
	void OnPlayerRespawn();

	/** 伤害数字动画完成回调（委托驱动，替代 Tick 轮询） */
	UFUNCTION()
	void OnDamageNumberFinished(UPP_DamageNumberWidget* Widget);
};
