// 任务系统 — 事件桥接器
// 监听全局游戏事件（采集/商店/聊天/战斗/对话）并路由到对应玩家的 QuestComponent

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Quest/Data/PP_QuestData.h"
#include "PP_QuestEventBridge.generated.h"

class UPP_QuestComponent;
class UPP_ShopComponent;

/**
 * 任务事件桥接器
 *
 * 职责：
 *   1. 在 GameMode（服务端）上运行，监听各子系统的全局事件
 *   2. 将事件翻译为 QuestComponent::UpdateObjectiveProgress 调用
 *   3. 支持的事件源：采集系统、商店系统、聊天系统、战斗系统、对话系统
 *
 * 生命周期：在 BeginPlay 中绑定所有子系统事件
 *
 * 使用约定：挂载在 APP_CrashGameMode 上。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PERSONALPROJECTQUEST_API UPP_QuestEventBridge : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_QuestEventBridge();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	// ===== 绑定接口 =====

	/** 订阅采集系统：EventBus 的 FPP_GatherEvent（替代原静态委托绑定） */
	void BindToGatherSystem();

	/** 绑定商店系统：遍历 NPC 绑定 ShopComponent->OnPurchaseCompleted */
	void BindToShopSystem();

	/** 绑定聊天系统：通过外部调用 NotifyChatSent 触发 */
	void BindToChatSystem();

	/** 绑定战斗系统：通过外部调用 NotifyEnemyKilled 触发 */
	void BindToCombatSystem();

	/** 绑定对话系统：通过外部调用 NotifyDialogueCompleted 触发 */
	void BindToDialogueSystem();

	// ===== 外部通知入口（由各子系统在事件发生时调用） =====

	/** 聊天消息发送通知 */
	UFUNCTION(BlueprintCallable, Category = "Quest|Bridge")
	void NotifyChatSent(APlayerController* Sender);

	/** 敌人被击杀通知 */
	UFUNCTION(BlueprintCallable, Category = "Quest|Bridge")
	void NotifyEnemyKilled(APlayerController* Killer, FName EnemyID);

	/** 对话完成通知 */
	UFUNCTION(BlueprintCallable, Category = "Quest|Bridge")
	void NotifyDialogueCompleted(APlayerController* Player, FName NPCID);

private:
	/** 采集完成事件回调（EventBus 订阅） */
	void OnGatherEvent(const struct FPP_GatherEvent& Event);

	/** 商店购买完成回调 */
	UFUNCTION()
	void OnPurchaseCompleted(AActor* Buyer, FName ItemID, int32 Count);

	/** 采集事件订阅句柄（EndPlay 时退订） */
	FDelegateHandle GatherEventHandle;

	/** 从 Actor 获取其 QuestComponent（通过 PlayerState） */
	UPP_QuestComponent* GetQuestComponentFromActor(AActor* Actor) const;

	/** 从 PlayerController 获取其 QuestComponent */
	UPP_QuestComponent* GetQuestComponentFromPC(APlayerController* PC) const;
};
