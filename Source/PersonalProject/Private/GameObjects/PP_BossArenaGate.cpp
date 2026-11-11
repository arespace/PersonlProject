
#include "GameObjects/PP_BossArenaGate.h"

#include "Characters/PP_BossCharacter.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Events/PP_EventBus.h"
#include "Events/PP_EventTypes.h"
#include "Framework/PP_Log.h"
#include "Net/UnrealNetwork.h"

APP_BossArenaGate::APP_BossArenaGate()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetupAttachment(MeshComponent);
	CollisionComponent->SetBoxExtent(FVector(100.f, 500.f, 300.f));
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(false);
}

void APP_BossArenaGate::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_BossArenaGate, bOpen);
}

void APP_BossArenaGate::SetLinkedBoss(APP_BossCharacter* InBoss)
{
	LinkedBoss = InBoss;
	LinkedBossName = IsValid(InBoss) ? InBoss->GetBossName() : NAME_None;
}

void APP_BossArenaGate::BeginPlay()
{
	Super::BeginPlay();

	// 蓝图里直接指定的 Boss 也要记录名字快照
	if (LinkedBoss && LinkedBossName.IsNone())
	{
		LinkedBossName = LinkedBoss->GetBossName();
	}

	// 初始化本地开门状态（客户端在收到复制前也按同一规则显示）
	bOpen = !bInitiallyClosed;
	ApplyOpenState(false);

	// 订阅 Boss 死亡事件 → 开门
	if (UPP_EventBus* Bus = UPP_EventBus::Get(this))
	{
		BossDeadHandle = Bus->Subscribe<FPP_BossDeadEvent>(
			[this](const FPP_BossDeadEvent& Event) { HandleBossDead(Event); });
	}
}

void APP_BossArenaGate::SetOpen(bool bInOpen)
{
	if (!HasAuthority())
	{
		return;
	}

	if (bOpen == bInOpen)
	{
		return;
	}

	bOpen = bInOpen;
	ApplyOpenState(bInOpen);
	ForceNetUpdate();
}

void APP_BossArenaGate::OnRep_bOpen()
{
	// 客户端：同步碰撞与开门表现
	ApplyOpenState(bOpen);
}

void APP_BossArenaGate::ApplyOpenState(bool bPlayAnim)
{
	// 门开 = 关闭阻挡碰撞
	SetActorEnableCollision(!bOpen);

	if (bOpen && bPlayAnim)
	{
		OnBossKilled();
	}
}

void APP_BossArenaGate::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UPP_EventBus* Bus = UPP_EventBus::Get(this))
	{
		Bus->Unsubscribe(BossDeadHandle);
		BossDeadHandle.Reset();
	}

	Super::EndPlay(EndPlayReason);
}

void APP_BossArenaGate::HandleBossDead(const FPP_BossDeadEvent& Event)
{
	// 只响应本场地关联 Boss 的死亡事件，避免多 Boss 关卡互相开错门
	if (LinkedBoss)
	{
		if (LinkedBoss->GetBossName() != Event.BossName)
		{
			UE_LOG(LogPP, Verbose, TEXT("[BossArenaGate] %s 忽略非本场地 Boss（%s）的死亡事件"),
				*GetName(), *Event.BossName.ToString());
			return;
		}
	}
	else
	{
		UE_LOG(LogPP, Warning,
			TEXT("[BossArenaGate] %s 未关联 Boss，收到任意 Boss（%s）死亡事件都会开门，请在蓝图配置或由 BossArenaTrigger 绑定"),
			*GetName(), *Event.BossName.ToString());
	}

	// 走统一入口：写 bOpen 并复制给客户端，客户端 OnRep 里同步碰撞与开门动画
	SetOpen(true);
}
