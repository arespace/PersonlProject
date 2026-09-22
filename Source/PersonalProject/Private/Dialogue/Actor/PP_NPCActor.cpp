// NPC对话系统 — NPC Actor 实现
// 负责组件构造、服务器权威对话启动、距离验证依赖 PlayerController 侧 RPC

#include "Dialogue/Actor/PP_NPCActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/DataTable.h"
#include "Dialogue/Data/PP_DialogueData.h"
#include "Characters/PP_CharacterData.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "Shop/Component/PP_ShopComponent.h"
#include "Quest/Component/PP_QuestComponent.h"

APP_NPCActor::APP_NPCActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	// 根组件：静态网格作为 NPC 的外观与碰撞主体
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);
	MeshComponent->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	// 骨骼网格组件：挂载在根组件上，用于显示人物角色模型 + 动画
	BodyMesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("BodyMesh"));
	BodyMesh->SetupAttachment(MeshComponent);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);  // 碰撞由根组件负责
	BodyMesh->SetVisibility(false);  // 默认隐藏，配置 NPCAppearance 后自动显示

	// 交互范围球体：仅 Query，用于交互提示与距离验证
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->SetupAttachment(RootComponent);
	InteractionRange->InitSphereRadius(250.f);
	InteractionRange->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionRange->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionRange->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 商店组件默认创建但不强制启用（无商品表则空跑）
	ShopComponent = CreateDefaultSubobject<UPP_ShopComponent>(TEXT("ShopComponent"));
}

void APP_NPCActor::Interact(APlayerController* Player)
{
	if (!IsValid(Player))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCActor] Interact - Player is null"));
		return;
	}

	// 服务器直接走 StartDialogue；客户端通过 PC 转发到 ServerRPC
	if (HasAuthority())
	{
		StartDialogue(Player);
		return;
	}

	APP_PlayerController* PPController = Cast<APP_PlayerController>(Player);
	if (PPController)
	{
		PPController->Server_RequestStartDialogue(this);
	}
}

FName APP_NPCActor::ResolveStartDialogueID(APP_PlayerController* Player) const
{
	if (!IsValid(Player) || !IsValid(DialogueTable))
	{
		return DefaultDialogueID;
	}

	APP_PlayerState* PS = Player->GetPlayerState<APP_PlayerState>();
	UPP_QuestComponent* QC = PS ? PS->GetQuestComponent() : nullptr;

	if (!QC)
	{
		return DefaultDialogueID;
	}

	const TMap<FName, uint8*>& RowMap = DialogueTable->GetRowMap();

	// 收集目标已全部完成的活跃任务 ID（后续轮次需跳过它们）
	TArray<FName> FullyCompletedIDs;
	UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] ===== NPC=%s ====="), *GetName());
	for (const FPP_ActiveQuest& AQ : QC->GetActiveQuests())
	{
		bool bAllDone = true;
		for (const FPP_QuestObjective& Obj : AQ.Objectives)
		{
			UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue]   Quest=%s Obj Type=%d Target=%s Cur=%d/%d Compl=%d"),
				*AQ.QuestID.ToString(), (int32)Obj.Type, *Obj.TargetID.ToString(), Obj.CurrentCount, Obj.RequiredCount, Obj.bCompleted ? 1 : 0);
			if (!Obj.bCompleted) { bAllDone = false; break; }
		}
		if (bAllDone) { FullyCompletedIDs.Add(AQ.QuestID); }
		UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue]   → Quest=%s AllDone=%d"), *AQ.QuestID.ToString(), bAllDone ? 1 : 0);
	}
	UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] ActiveQuests=%d, FullyCompleted=%d"),
		QC->GetActiveQuests().Num(), FullyCompletedIDs.Num());

	// === 第一轮：目标已全完成的活跃任务 → 提交/完成对话 ===
	for (const FName& QID : FullyCompletedIDs)
	{
		UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] Round1 checking QID=%s"), *QID.ToString());

		// 1a. RequireQuestActiveID + SubmitQuest（精确匹配：目标完成、待提交）
		for (const auto& Pair : RowMap)
		{
			FPP_DialogueData* Row = reinterpret_cast<FPP_DialogueData*>(Pair.Value);
			if (!Row) continue;
			if (Row->RequireQuestActiveID == QID
				&& Row->EndAction == EPP_DialogueEndAction::SubmitQuest)
			{
				FName Result = Row->DialogueID.IsNone() ? Pair.Key : Row->DialogueID;
				UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] Round1a MATCH: QID=%s → %s (EndAction=SubmitQuest)"), *QID.ToString(), *Result.ToString());
				return Result;
			}
		}

		// 1b. RequireQuestCompletedID（兼容：用户可能用此字段配置提交对话）
		for (const auto& Pair : RowMap)
		{
			FPP_DialogueData* Row = reinterpret_cast<FPP_DialogueData*>(Pair.Value);
			if (!Row) continue;
			if (Row->RequireQuestCompletedID == QID)
			{
				FName Result = Row->DialogueID.IsNone() ? Pair.Key : Row->DialogueID;
				UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] Round1b MATCH: QID=%s → %s (via RequireCompleted)"), *QID.ToString(), *Result.ToString());
				return Result;
			}
		}
	}

	// === 第二轮：活跃任务但目标未全部完成 → 进行中对话 ===
	for (const auto& Pair : RowMap)
	{
		FPP_DialogueData* Row = reinterpret_cast<FPP_DialogueData*>(Pair.Value);
		if (!Row) continue;
		if (!Row->RequireQuestActiveID.IsNone()
			&& QC->IsQuestActive(Row->RequireQuestActiveID)
			&& !FullyCompletedIDs.Contains(Row->RequireQuestActiveID))
		{
			FName Result = Row->DialogueID.IsNone() ? Pair.Key : Row->DialogueID;
			UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] Round2 MATCH: ReqActive=%s → %s (EndAction=%d)"),
				*Row->RequireQuestActiveID.ToString(), *Result.ToString(), (int32)Row->EndAction);
			return Result;
		}
	}

	// === 第三轮：已提交完成的任务 → 后续对话 ===
	for (const auto& Pair : RowMap)
	{
		FPP_DialogueData* Row = reinterpret_cast<FPP_DialogueData*>(Pair.Value);
		if (!Row) continue;
		if (!Row->RequireQuestCompletedID.IsNone()
			&& QC->IsQuestCompleted(Row->RequireQuestCompletedID))
		{
			if (Row->EndAction == EPP_DialogueEndAction::OfferQuest
				&& !Row->RelatedQuestID.IsNone()
				&& QC->IsQuestCompleted(Row->RelatedQuestID))
			{
				continue;
			}
			FName Result = Row->DialogueID.IsNone() ? Pair.Key : Row->DialogueID;
			UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] Round3 MATCH: ReqCompleted=%s → %s"),
				*Row->RequireQuestCompletedID.ToString(), *Result.ToString());
			return Result;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[ResolveDialogue] NO MATCH → Default: %s"), *DefaultDialogueID.ToString());
	return DefaultDialogueID;
}

void APP_NPCActor::StartDialogue(APlayerController* Player)
{
	if (!HasAuthority())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCActor] StartDialogue called on client - ignored (must run on server)"));
		return;
	}
	if (!IsValid(Player))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCActor] StartDialogue - Player is null"));
		return;
	}
	if (!IsValid(DialogueTable))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCActor] StartDialogue - DialogueTable not configured on %s"), *GetName());
		return;
	}

	APP_PlayerController* PPController = Cast<APP_PlayerController>(Player);
	if (!IsValid(PPController))
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCActor] StartDialogue - Player is not APP_PlayerController"));
		return;
	}

	const FName StartID = ResolveStartDialogueID(PPController);
	if (StartID.IsNone())
	{
		UE_LOG(LogTemp, Warning, TEXT("[NPCActor] StartDialogue - resolved DialogueID is None on %s"), *GetName());
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[NPCActor] StartDialogue NPC=%s Player=%s DialogueID=%s"),
		*GetName(), *PPController->GetName(), *StartID.ToString());

	// 通过 PC 的 ClientRPC 触发客户端 UI 打开
	PPController->Client_OpenDialogueUI(this, StartID);
}

// ===== 角色外观 =====

void APP_NPCActor::BeginPlay()
{
	Super::BeginPlay();
	ApplyNPCAppearance();
}

void APP_NPCActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	ApplyNPCAppearance();
}

void APP_NPCActor::ApplyNPCAppearance()
{
	if (!BodyMesh)
	{
		return;
	}

	// 检查是否配置了角色外观
	USkeletalMesh* LoadedMesh = NPCAppearance.CharacterMesh.LoadSynchronous();
	if (!LoadedMesh)
	{
		// 未配置：隐藏骨骼网格，显示静态网格（向后兼容）
		BodyMesh->SetVisibility(false);
		if (MeshComponent)
		{
			MeshComponent->SetVisibility(true);
		}
		return;
	}

	// 已配置：应用角色外观，显示骨骼网格，隐藏静态网格
	BodyMesh->SetSkeletalMesh(LoadedMesh);
	BodyMesh->SetVisibility(true);
	BodyMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);  // 开启碰撞，使 Interact 的 LineTrace 能命中
	BodyMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	BodyMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	if (UClass* AnimClass = NPCAppearance.AnimBlueprintClass.LoadSynchronous())
	{
		BodyMesh->SetAnimInstanceClass(AnimClass);
	}

	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false);
	}
}
