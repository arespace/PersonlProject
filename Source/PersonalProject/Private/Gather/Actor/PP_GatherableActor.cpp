// 采集系统 — 可交互采集物Actor 实现

#include "Gather/Actor/PP_GatherableActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/Component/PP_InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/PP_PlayerState.h"
#include "TimerManager.h"

// 全局静态委托定义
FOnGatherCompleted APP_GatherableActor::OnGatherCompletedDelegate;

APP_GatherableActor::APP_GatherableActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	// 根组件：交互范围球（同时承担根节点角色）
	InteractionRange = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionRange"));
	InteractionRange->InitSphereRadius(200.0f);
	InteractionRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	InteractionRange->SetGenerateOverlapEvents(true);
	RootComponent = InteractionRange;

	// 可视网格组件
	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	MeshComponent->SetupAttachment(RootComponent);
	MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	MeshComponent->SetCollisionResponseToAllChannels(ECR_Block);

	RemainingGatherTimes = 0;
}

void APP_GatherableActor::BeginPlay()
{
	Super::BeginPlay();

	// 服务器初始化剩余采集次数
	if (HasAuthority())
	{
		RemainingGatherTimes = MaxGatherTimes;
	}

	// Overlap 事件绑定（按规范在 BeginPlay 中设置）
	if (InteractionRange)
	{
		InteractionRange->OnComponentBeginOverlap.AddDynamic(this, &APP_GatherableActor::OnInteractionRangeBeginOverlap);
		InteractionRange->OnComponentEndOverlap.AddDynamic(this, &APP_GatherableActor::OnInteractionRangeEndOverlap);
	}
}

void APP_GatherableActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_GatherableActor, CurrentGatherer);
	DOREPLIFETIME(APP_GatherableActor, RemainingGatherTimes);
}

bool APP_GatherableActor::IsAvailable() const
{
	return CurrentGatherer == nullptr && RemainingGatherTimes > 0;
}

bool APP_GatherableActor::CanInteract(APlayerController* Interactor) const
{
	return IsAvailable();
}

void APP_GatherableActor::Interact(APlayerController* Interactor)
{
	if (!Interactor || !IsAvailable())
	{
		return;
	}

	APawn* Pawn = Interactor->GetPawn();
	if (!Pawn)
	{
		return;
	}

	// 客户端通过 Server RPC 发起采集；服务器直接执行
	if (HasAuthority())
	{
		Server_StartGather_Implementation(Pawn);
	}
	else
	{
		Server_StartGather(Pawn);
	}
}

void APP_GatherableActor::Server_StartGather_Implementation(AActor* Gatherer)
{
	if (!HasAuthority() || !Gatherer)
	{
		return;
	}

	if (!IsAvailable())
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Gather] Server_StartGather refused: actor=%s available=false"),
			*GetName());
		return;
	}

	CurrentGatherer = Gatherer;
	UE_LOG(LogTemp, Log, TEXT("[Gather] Server_StartGather accepted: actor=%s gatherer=%s remaining=%d"),
		*GetName(), *Gatherer->GetName(), RemainingGatherTimes);
}

void APP_GatherableActor::Server_CancelGather_Implementation(AActor* Gatherer)
{
	if (!HasAuthority())
	{
		return;
	}

	// 仅当前采集者本人可取消
	if (CurrentGatherer == Gatherer)
	{
		UE_LOG(LogTemp, Log, TEXT("[Gather] Server_CancelGather: actor=%s gatherer=%s"),
			*GetName(), Gatherer ? *Gatherer->GetName() : TEXT("null"));
		CurrentGatherer = nullptr;
	}
}

void APP_GatherableActor::NotifyPickup(AActor* Gatherer)
{
	if (!HasAuthority() || !Gatherer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gather] NotifyPickup SKIP - HasAuthority=%d Gatherer=%s"),
			HasAuthority() ? 1 : 0, *GetNameSafe(Gatherer));
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Gather] NotifyPickup: actor=%s ItemID=%s GatherCount=%d Gatherer=%s"),
		*GetName(), *ItemID.ToString(), GatherCount, *Gatherer->GetName());

	// 广播采集完成事件（任务系统通过静态委托监听）
	OnGatherCompletedDelegate.Broadcast(Gatherer, ItemID, GatherCount);
}

void APP_GatherableActor::CompleteGather(AActor* Gatherer)
{
	if (!HasAuthority() || !Gatherer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gather] CompleteGather SKIP - HasAuthority=%d Gatherer=%s"),
			HasAuthority() ? 1 : 0, *GetNameSafe(Gatherer));
		return;
	}

	// 校验：当前采集者必须是发起者，且仍有剩余次数
	if (CurrentGatherer != Gatherer || RemainingGatherTimes <= 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gather] CompleteGather rejected: actor=%s mismatch=%d depleted=%d (CurrentGatherer=%s Remaining=%d)"),
			*GetName(), (CurrentGatherer != Gatherer) ? 1 : 0, (RemainingGatherTimes <= 0) ? 1 : 0,
			*GetNameSafe(CurrentGatherer), RemainingGatherTimes);
		return;
	}

	UE_LOG(LogTemp, Warning, TEXT("[Gather] CompleteGather START: actor=%s ItemID=%s GatherCount=%d"),
		*GetName(), *ItemID.ToString(), GatherCount);

	// 1. 通过玩家PlayerState上的库存组件添加物品
	UPP_InventoryComponent* Inventory = nullptr;
	if (APawn* Pawn = Cast<APawn>(Gatherer))
	{
		if (APP_PlayerState* PS = Pawn->GetPlayerState<APP_PlayerState>())
		{
			Inventory = PS->GetInventoryComponent();
		}
	}

	if (Inventory)
	{
		const bool bAdded = Inventory->AddItem(ItemID, GatherCount);
		UE_LOG(LogTemp, Log, TEXT("[Gather] CompleteGather AddItem: itemID=%s count=%d success=%d"),
			*ItemID.ToString(), GatherCount, bAdded ? 1 : 0);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[Gather] CompleteGather: gatherer=%s missing InventoryComponent"),
			*Gatherer->GetName());
	}

	// 2. 扣减剩余次数并清空当前采集者
	--RemainingGatherTimes;
	CurrentGatherer = nullptr;

	// 3. 广播全局委托（任务系统等监听）
	OnGatherCompletedDelegate.Broadcast(Gatherer, ItemID, GatherCount);

	// 4. 检查是否耗尽
	if (RemainingGatherTimes <= 0)
	{
		Deplete();
	}
}

void APP_GatherableActor::Deplete()
{
	UE_LOG(LogTemp, Log, TEXT("[Gather] Deplete: actor=%s scheduling respawn in %.1fs"),
		*GetName(), RespawnTime);

	// 隐藏网格 + 关闭交互范围检测
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(false);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
	if (InteractionRange)
	{
		InteractionRange->SetGenerateOverlapEvents(false);
	}

	// 启动重生计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(RespawnTimerHandle, this, &APP_GatherableActor::Respawn, RespawnTime, false);
	}
}

void APP_GatherableActor::Respawn()
{
	if (!HasAuthority())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("[Gather] Respawn: actor=%s"), *GetName());

	// 恢复采集次数
	RemainingGatherTimes = MaxGatherTimes;

	// 恢复网格可见与交互范围检测
	if (MeshComponent)
	{
		MeshComponent->SetVisibility(true);
		MeshComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	}
	if (InteractionRange)
	{
		InteractionRange->SetGenerateOverlapEvents(true);
	}
}

void APP_GatherableActor::OnInteractionRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 仅响应Pawn，且当前节点可用时才提示
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn || !IsAvailable())
	{
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Gather] BeginOverlap: actor=%s pawn=%s"),
		*GetName(), *OtherActor->GetName());
	// 注：UI提示由PlayerController监听Overlap事件或通过GameplayCue实现，此处仅做日志记录
}

void APP_GatherableActor::OnInteractionRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	UE_LOG(LogTemp, Verbose, TEXT("[Gather] EndOverlap: actor=%s pawn=%s"),
		*GetName(), *OtherActor->GetName());
}
