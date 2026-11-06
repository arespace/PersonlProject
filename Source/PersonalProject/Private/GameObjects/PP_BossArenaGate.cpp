
#include "GameObjects/PP_BossArenaGate.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Events/PP_EventBus.h"
#include "Events/PP_EventTypes.h"

APP_BossArenaGate::APP_BossArenaGate()
{
	PrimaryActorTick.bCanEverTick = false;

	MeshComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComponent"));
	SetRootComponent(MeshComponent);

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetupAttachment(MeshComponent);
	CollisionComponent->SetBoxExtent(FVector(100.f, 500.f, 300.f));
	CollisionComponent->SetCollisionProfileName(TEXT("BlockAllDynamic"));
	CollisionComponent->SetGenerateOverlapEvents(false);
}

void APP_BossArenaGate::BeginPlay()
{
	Super::BeginPlay();

	// 初始开门（禁用碰撞），等 BossArenaTrigger 关闭
	if (!bInitiallyClosed)
	{
		SetActorEnableCollision(false);
	}

	// 订阅 Boss 死亡事件 → 开门
	if (UPP_EventBus* Bus = UPP_EventBus::Get(this))
	{
		BossDeadHandle = Bus->Subscribe<FPP_BossDeadEvent>(
			[this](const FPP_BossDeadEvent& Event) { HandleBossDead(Event); });
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
	// 解锁碰撞 → 开门
	SetActorEnableCollision(false);

	// 蓝图播放开门动画
	OnBossKilled();
}
