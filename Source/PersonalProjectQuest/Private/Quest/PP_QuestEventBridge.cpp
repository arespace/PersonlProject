// 任务系统 — 事件桥接器实现
// 绑定各子系统事件并路由到对应玩家的 QuestComponent

#include "Quest/PP_QuestEventBridge.h"

#include "Quest/PP_QuestComponent.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "GameFramework/PlayerState.h"
#include "Shop/PP_ShopComponent.h"
#include "Dialogue/PP_NPCActor.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"

UPP_QuestEventBridge::UPP_QuestEventBridge()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPP_QuestEventBridge::BeginPlay()
{
	Super::BeginPlay();

	// 仅在服务器上运行绑定
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	BindToGatherSystem();
	BindToShopSystem();
	BindToChatSystem();
	BindToCombatSystem();
	BindToDialogueSystem();

	UE_LOG(LogPP, Log, TEXT("[QuestEventBridge] All subsystem bindings completed"));
}

void UPP_QuestEventBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 退订 EventBus 事件（Bridge 生命周期短于 GameInstance，必须成对退订）
	if (UWorld* World = GetWorld())
	{
		if (UPP_EventBus* EventBus = World->GetGameInstance()->GetSubsystem<UPP_EventBus>())
		{
			EventBus->Unsubscribe(GatherEventHandle);
		}
	}
	Super::EndPlay(EndPlayReason);
}

// ===== 绑定实现 =====

void UPP_QuestEventBridge::BindToGatherSystem()
{
	// 订阅 EventBus 采集完成事件（替代原全局静态委托，采集系统与任务系统解耦）
	if (UWorld* World = GetWorld())
	{
		if (UPP_EventBus* EventBus = World->GetGameInstance()->GetSubsystem<UPP_EventBus>())
		{
			GatherEventHandle = EventBus->Subscribe<FPP_GatherEvent>(
				[this](const FPP_GatherEvent& Event)
				{
					OnGatherEvent(Event);
				});
		}
	}
	UE_LOG(LogPP, Log, TEXT("[QuestEventBridge] Bound to GatherSystem (EventBus)"));
}

void UPP_QuestEventBridge::BindToShopSystem()
{
	// 绑定静态委托，不依赖遍历 NPC 实例（参照 Gather 系统模式，避免 BeginPlay 时序问题）
	UPP_ShopComponent::OnPurchaseCompletedDelegate.AddDynamic(this, &UPP_QuestEventBridge::OnPurchaseCompleted);
	UE_LOG(LogPP, Log, TEXT("[QuestEventBridge] Bound to ShopSystem (static delegate)"));
}

void UPP_QuestEventBridge::BindToChatSystem()
{
	// 聊天系统通过 NotifyChatSent 被 PP_PlayerController 主动调用
	UE_LOG(LogPP, Log, TEXT("[QuestEventBridge] ChatSystem ready (push-based via NotifyChatSent)"));
}

void UPP_QuestEventBridge::BindToCombatSystem()
{
	// 战斗系统通过 NotifyEnemyKilled 被 PP_EnemyCharacter::HandleDeath 主动调用
	UE_LOG(LogPP, Log, TEXT("[QuestEventBridge] CombatSystem ready (push-based via NotifyEnemyKilled)"));
}

void UPP_QuestEventBridge::BindToDialogueSystem()
{
	// 对话系统通过 NotifyDialogueCompleted 被 PC 主动调用
	UE_LOG(LogPP, Log, TEXT("[QuestEventBridge] DialogueSystem ready (push-based via NotifyDialogueCompleted)"));
}

// ===== 事件回调 =====

void UPP_QuestEventBridge::OnGatherEvent(const FPP_GatherEvent& Event)
{
	UE_LOG(LogPP, Verbose, TEXT("[QuestEventBridge] OnGatherEvent: Gatherer=%s ItemID=%s Count=%d"),
		*GetNameSafe(Event.Gatherer), *Event.ItemID.ToString(), Event.Count);

	// 事件中的 Gatherer 已是 PlayerController
	UPP_QuestComponent* QC = GetQuestComponentFromPC(Event.Gatherer);
	if (!QC)
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestEventBridge] OnGatherEvent - QC is NULL for Gatherer=%s"), *GetNameSafe(Event.Gatherer));
		return;
	}

	UE_LOG(LogPP, Verbose, TEXT("[QuestEventBridge] OnGatherEvent - Calling UpdateObjectiveProgress: Type=GatherItem TargetID=%s Count=%d"),
		*Event.ItemID.ToString(), Event.Count);
	QC->UpdateObjectiveProgress(EPP_QuestObjectiveType::GatherItem, Event.ItemID, Event.Count);
}

void UPP_QuestEventBridge::OnPurchaseCompleted(AActor* Buyer, FName ItemID, int32 Count)
{
	UPP_QuestComponent* QC = GetQuestComponentFromActor(Buyer);
	if (QC)
	{
		QC->UpdateObjectiveProgress(EPP_QuestObjectiveType::PurchaseItem, ItemID, Count);
	}
}

// ===== 外部通知入口 =====

void UPP_QuestEventBridge::NotifyChatSent(APlayerController* Sender)
{
	UPP_QuestComponent* QC = GetQuestComponentFromPC(Sender);
	if (QC)
	{
		QC->UpdateObjectiveProgress(EPP_QuestObjectiveType::SendChat, FName("Chat"), 1);
	}
}

void UPP_QuestEventBridge::NotifyEnemyKilled(APlayerController* Killer, FName EnemyID)
{
	UPP_QuestComponent* QC = GetQuestComponentFromPC(Killer);
	if (QC)
	{
		QC->UpdateObjectiveProgress(EPP_QuestObjectiveType::KillEnemy, EnemyID, 1);

		// 检查是否有共享目标需要广播给队友
		for (const FPP_ActiveQuest& Quest : QC->GetActiveQuests())
		{
			for (const FPP_QuestObjective& Obj : Quest.Objectives)
			{
				if (Obj.Type == EPP_QuestObjectiveType::KillEnemy && Obj.TargetID == EnemyID && Obj.bSharedInTeam)
				{
					QC->Server_BroadcastTeamProgress(EPP_QuestObjectiveType::KillEnemy, EnemyID, 1);
					return; // 一次击杀只广播一次
				}
			}
		}
	}
}

void UPP_QuestEventBridge::NotifyDialogueCompleted(APlayerController* Player, FName NPCID)
{
	UPP_QuestComponent* QC = GetQuestComponentFromPC(Player);
	if (QC)
	{
		QC->UpdateObjectiveProgress(EPP_QuestObjectiveType::TalkToNPC, NPCID, 1);
	}
}

// ===== 内部辅助 =====

UPP_QuestComponent* UPP_QuestEventBridge::GetQuestComponentFromActor(AActor* Actor) const
{
	if (!IsValid(Actor)) return nullptr;

	// Actor 可能是 Controller（如 Shop 的 Server_BuyItem 传 PlayerController）
	if (AController* Controller = Cast<AController>(Actor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Controller))
		{
			return GetQuestComponentFromPC(PC);
		}
	}

	// Actor 可能是 Pawn（如 Gather 系统传 Pawn/Character）
	if (APawn* Pawn = Cast<APawn>(Actor))
	{
		if (APlayerController* PC = Cast<APlayerController>(Pawn->GetController()))
		{
			return GetQuestComponentFromPC(PC);
		}
	}

	// 尝试直接在 Actor 上找
	return Actor->FindComponentByClass<UPP_QuestComponent>();
}

UPP_QuestComponent* UPP_QuestEventBridge::GetQuestComponentFromPC(APlayerController* PC) const
{
	if (!IsValid(PC)) return nullptr;

	APlayerState* PS = PC->GetPlayerState<APlayerState>();
	if (!IsValid(PS)) return nullptr;

	return PS->FindComponentByClass<UPP_QuestComponent>();
}
