// 任务系统 — 任务管理组件实现
// 处理任务接取/放弃/提交/进度更新/奖励发放的核心逻辑

#include "Quest/PP_QuestComponent.h"

#include "Framework/PP_DataManager.h"
#include "Framework/PP_EventBus.h"
#include "Framework/PP_EventTypes.h"
#include "GameFramework/PlayerState.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerController.h"
#include "Framework/PP_Log.h"

UPP_QuestComponent::UPP_QuestComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UPP_QuestComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UPP_QuestComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(UPP_QuestComponent, ActiveQuests);
	DOREPLIFETIME(UPP_QuestComponent, CompletedQuestIDs);
}

// ===== IPP_QuestQuery 接口实现 =====

TArray<FPP_ActiveQuest> UPP_QuestComponent::QueryActiveQuests_Implementation()
{
	return ActiveQuests;
}

TArray<FName> UPP_QuestComponent::QueryCompletedQuests_Implementation()
{
	return CompletedQuestIDs;
}

bool UPP_QuestComponent::QueryQuestInfo_Implementation(FName QuestID, FPP_QuestInfo& OutInfo)
{
	const FPP_QuestInfo* QuestInfo = FindQuestInfo(QuestID);
	if (!QuestInfo)
	{
		return false;
	}
	OutInfo = *QuestInfo;
	return true;
}

// ===== 任务操作接口 =====

bool UPP_QuestComponent::AcceptQuest(FName QuestID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestComponent] AcceptQuest - Not server authority"));
		return false;
	}

	if (!CanAcceptQuest(QuestID))
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestComponent] AcceptQuest - Cannot accept quest '%s'"), *QuestID.ToString());
		return false;
	}

	const FPP_QuestInfo* QuestInfo = FindQuestInfo(QuestID);
	if (!QuestInfo)
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestComponent] AcceptQuest - Quest '%s' not found in DataTable"), *QuestID.ToString());
		return false;
	}

	// 创建活跃任务
	FPP_ActiveQuest NewActiveQuest;
	NewActiveQuest.QuestID = QuestID;
	NewActiveQuest.Objectives = QuestInfo->Objectives;

	// 重置运行时状态
	for (FPP_QuestObjective& Obj : NewActiveQuest.Objectives)
	{
		Obj.CurrentCount = 0;
		Obj.bCompleted = false;
	}

	ActiveQuests.Add(NewActiveQuest);
	BroadcastQuestsUpdatedEvent();

	UE_LOG(LogPP, Log, TEXT("[QuestComponent] AcceptQuest - Accepted '%s'"), *QuestID.ToString());
	return true;
}

bool UPP_QuestComponent::SubmitQuest(FName QuestID)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestComponent] SubmitQuest - Not server authority"));
		return false;
	}

	// 查找活跃任务
	int32 QuestIndex = INDEX_NONE;
	for (int32 i = 0; i < ActiveQuests.Num(); ++i)
	{
		if (ActiveQuests[i].QuestID == QuestID)
		{
			QuestIndex = i;
			break;
		}
	}

	if (QuestIndex == INDEX_NONE)
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestComponent] SubmitQuest - Quest '%s' not active"), *QuestID.ToString());
		return false;
	}

	// 验证所有目标已完成
	for (const FPP_QuestObjective& Obj : ActiveQuests[QuestIndex].Objectives)
	{
		if (!Obj.bCompleted)
		{
			UE_LOG(LogPP, Warning, TEXT("[QuestComponent] SubmitQuest - Quest '%s' has incomplete objectives"), *QuestID.ToString());
			return false;
		}
	}

	// 发放奖励
	const FPP_QuestInfo* QuestInfo = FindQuestInfo(QuestID);
	if (QuestInfo)
	{
		UPP_InventoryComponent* Inventory = GetOwningInventoryComponent();
		if (Inventory)
		{
			// 金币奖励
			if (QuestInfo->Reward.GoldReward > 0)
			{
				Inventory->AddGold(QuestInfo->Reward.GoldReward);
			}

			// 物品奖励
			for (const FPP_ItemInstance& Item : QuestInfo->Reward.ItemRewards)
			{
				if (!Item.ItemID.IsNone() && Item.Count > 0)
				{
					Inventory->AddItem(Item.ItemID, Item.Count);
				}
			}
		}
	}

	// 移入已完成列表
	CompletedQuestIDs.Add(QuestID);
	ActiveQuests.RemoveAt(QuestIndex);

	BroadcastQuestsUpdatedEvent();

	// 通过 EventBus 广播任务完成事件（HUD 订阅显示通知，任务系统不再直接依赖 UI）
	FPP_QuestCompletedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	Event.QuestID = QuestID;
	Event.QuestName = QuestInfo ? QuestInfo->QuestName.ToString() : QuestID.ToString();
	UPP_EventBus::BroadcastEvent(this, Event);

	// 解锁下一主线任务（自动接取由 NPC 对话触发，此处仅记录日志）
	if (QuestInfo && !QuestInfo->NextMainQuestID.IsNone())
	{
		UE_LOG(LogPP, Log, TEXT("[QuestComponent] SubmitQuest - NextMainQuestID '%s' now unlocked"),
			*QuestInfo->NextMainQuestID.ToString());
	}

	UE_LOG(LogPP, Log, TEXT("[QuestComponent] SubmitQuest - Completed '%s'"), *QuestID.ToString());
	return true;
}

void UPP_QuestComponent::UpdateObjectiveProgress(EPP_QuestObjectiveType Type, FName TargetID, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		UE_LOG(LogPP, Warning, TEXT("[QuestComponent] UpdateObjectiveProgress SKIP - Not authority"));
		return;
	}

	UE_LOG(LogPP, Verbose, TEXT("[QuestComponent] UpdateObjectiveProgress ENTER: Type=%d TargetID=%s Count=%d ActiveQuests=%d"),
		(int32)Type, *TargetID.ToString(), Count, ActiveQuests.Num());

	bool bMatched = false;
	for (int32 QIdx = 0; QIdx < ActiveQuests.Num(); ++QIdx)
	{
		FPP_ActiveQuest& Quest = ActiveQuests[QIdx];
		for (int32 OIdx = 0; OIdx < Quest.Objectives.Num(); ++OIdx)
		{
			FPP_QuestObjective& Obj = Quest.Objectives[OIdx];

			UE_LOG(LogPP, Verbose, TEXT("[QuestComponent] Checking Quest=%s Obj[%d]: Type=%d TargetID=%s Compl=%d Cur=%d/%d"),
				*Quest.QuestID.ToString(), OIdx, (int32)Obj.Type, *Obj.TargetID.ToString(),
				Obj.bCompleted ? 1 : 0, Obj.CurrentCount, Obj.RequiredCount);

			if (Obj.bCompleted) continue;
			if (Obj.Type != Type) continue;
			if (Obj.TargetID != TargetID) continue;

			bMatched = true;
			Obj.CurrentCount = FMath::Min(Obj.CurrentCount + Count, Obj.RequiredCount);
			if (Obj.CurrentCount >= Obj.RequiredCount)
			{
				Obj.bCompleted = true;
			}

			// 广播 UI 更新（EventBus）
			BroadcastQuestsUpdatedEvent();

			UE_LOG(LogPP, Verbose, TEXT("[QuestComponent] UpdateObjectiveProgress MATCH: Quest='%s' Obj[%d] %d/%d"),
				*Quest.QuestID.ToString(), OIdx, Obj.CurrentCount, Obj.RequiredCount);
		}
	}

	if (bMatched)
	{
		// 强制复制：TArray 内部元素修改不会触发 OnRep，需重新赋值让复制系统感知变更
		TArray<FPP_ActiveQuest> Temp = ActiveQuests;
		ActiveQuests = MoveTemp(Temp);
	}
	else
	{
		UE_LOG(LogPP, Verbose, TEXT("[QuestComponent] UpdateObjectiveProgress NO MATCH: Type=%d TargetID=%s"),
			(int32)Type, *TargetID.ToString());
	}
}

void UPP_QuestComponent::Server_BroadcastTeamProgress(EPP_QuestObjectiveType Type, FName TargetID, int32 Count)
{
	if (!GetOwner() || !GetOwner()->HasAuthority())
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World) return;

	// 遍历所有 PlayerState 找到队友的 QuestComponent 并更新
	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		APlayerController* PC = It->Get();
		if (!IsValid(PC)) continue;

		APlayerState* PS = PC->GetPlayerState<APlayerState>();
		if (!IsValid(PS)) continue;

		// 跳过自己
		if (PS == GetOwner()) continue;

		UPP_QuestComponent* TeamQC = PS->FindComponentByClass<UPP_QuestComponent>();
		if (TeamQC)
		{
			TeamQC->UpdateObjectiveProgress(Type, TargetID, Count);
		}
	}
}

// ===== 查询接口 =====

bool UPP_QuestComponent::IsQuestActive(FName QuestID) const
{
	for (const FPP_ActiveQuest& Quest : ActiveQuests)
	{
		if (Quest.QuestID == QuestID)
		{
			return true;
		}
	}
	return false;
}

bool UPP_QuestComponent::IsQuestCompleted(FName QuestID) const
{
	return CompletedQuestIDs.Contains(QuestID);
}

bool UPP_QuestComponent::CanAcceptQuest(FName QuestID) const
{
	// 已接取或已完成则不可重复接取
	if (IsQuestActive(QuestID) || IsQuestCompleted(QuestID))
	{
		return false;
	}

	// 检查前置任务
	const FPP_QuestInfo* QuestInfo = FindQuestInfo(QuestID);
	if (!QuestInfo)
	{
		return false;
	}

	for (const FName& PrereqID : QuestInfo->PrerequisiteQuestIDs)
	{
		if (!IsQuestCompleted(PrereqID))
		{
			return false;
		}
	}

	return true;
}

const FPP_QuestInfo* UPP_QuestComponent::FindQuestInfo(FName QuestID) const
{
	// 统一走 DataManager 全局任务表
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	if (!DataManager) return nullptr;
	return DataManager->GetQuestInfo(QuestID);
}

// ===== OnRep =====

void UPP_QuestComponent::OnRep_ActiveQuests()
{
	// 客户端收到活跃任务列表更新后广播，通知 UI 刷新
	UE_LOG(LogPP, Log, TEXT("[QuestComponent] OnRep_ActiveQuests - %d active quests"), ActiveQuests.Num());
	BroadcastQuestsUpdatedEvent();
}

// ===== 内部辅助 =====

void UPP_QuestComponent::BroadcastQuestsUpdatedEvent()
{
	FPP_QuestsUpdatedEvent Event;
	const APlayerState* OwnerPS = Cast<APlayerState>(GetOwner());
	Event.Player = OwnerPS ? OwnerPS->GetPlayerController() : nullptr;
	UPP_EventBus::BroadcastEvent(this, Event);
}

UPP_InventoryComponent* UPP_QuestComponent::GetOwningInventoryComponent() const
{
	AActor* Owner = GetOwner();
	if (!Owner) return nullptr;
	return Owner->FindComponentByClass<UPP_InventoryComponent>();
}
