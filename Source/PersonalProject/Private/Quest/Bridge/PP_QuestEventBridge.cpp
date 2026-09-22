// 任务系统 — 事件桥接器实现
// 绑定各子系统事件并路由到对应玩家的 QuestComponent

#include "Quest/Bridge/PP_QuestEventBridge.h"

#include "Quest/Component/PP_QuestComponent.h"
#include "Gather/Actor/PP_GatherableActor.h"
#include "Shop/Component/PP_ShopComponent.h"
#include "Dialogue/Actor/PP_NPCActor.h"
#include "Player/PP_PlayerState.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

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

	UE_LOG(LogTemp, Log, TEXT("[QuestEventBridge] All subsystem bindings completed"));
}

// ===== 绑定实现 =====

void UPP_QuestEventBridge::BindToGatherSystem()
{
	// 绑定全局静态委托
	APP_GatherableActor::OnGatherCompletedDelegate.AddDynamic(this, &UPP_QuestEventBridge::OnGatherCompleted);
	UE_LOG(LogTemp, Log, TEXT("[QuestEventBridge] Bound to GatherSystem"));
}

void UPP_QuestEventBridge::BindToShopSystem()
{
	// 绑定静态委托，不依赖遍历 NPC 实例（参照 Gather 系统模式，避免 BeginPlay 时序问题）
	UPP_ShopComponent::OnPurchaseCompletedDelegate.AddDynamic(this, &UPP_QuestEventBridge::OnPurchaseCompleted);
	UE_LOG(LogTemp, Log, TEXT("[QuestEventBridge] Bound to ShopSystem (static delegate)"));
}

void UPP_QuestEventBridge::BindToChatSystem()
{
	// 聊天系统通过 NotifyChatSent 被 PP_PlayerController 主动调用
	UE_LOG(LogTemp, Log, TEXT("[QuestEventBridge] ChatSystem ready (push-based via NotifyChatSent)"));
}

void UPP_QuestEventBridge::BindToCombatSystem()
{
	// 战斗系统通过 NotifyEnemyKilled 被 PP_EnemyCharacter::HandleDeath 主动调用
	UE_LOG(LogTemp, Log, TEXT("[QuestEventBridge] CombatSystem ready (push-based via NotifyEnemyKilled)"));
}

void UPP_QuestEventBridge::BindToDialogueSystem()
{
	// 对话系统通过 NotifyDialogueCompleted 被 PC 主动调用
	UE_LOG(LogTemp, Log, TEXT("[QuestEventBridge] DialogueSystem ready (push-based via NotifyDialogueCompleted)"));
}

// ===== 事件回调 =====

void UPP_QuestEventBridge::OnGatherCompleted(AActor* Gatherer, FName ItemID, int32 Count)
{
	UE_LOG(LogTemp, Warning, TEXT("[QuestEventBridge] OnGatherCompleted: Gatherer=%s ItemID=%s Count=%d"),
		*GetNameSafe(Gatherer), *ItemID.ToString(), Count);

	UPP_QuestComponent* QC = GetQuestComponentFromActor(Gatherer);
	if (!QC)
	{
		UE_LOG(LogTemp, Warning, TEXT("[QuestEventBridge] OnGatherCompleted - QC is NULL for Gatherer=%s"), *GetNameSafe(Gatherer));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[QuestEventBridge] OnGatherCompleted - Calling UpdateObjectiveProgress: Type=GatherItem TargetID=%s Count=%d"),
		*ItemID.ToString(), Count);
	QC->UpdateObjectiveProgress(EPP_QuestObjectiveType::GatherItem, ItemID, Count);
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
