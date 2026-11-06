
#include "GameObjects/PP_BossArenaTrigger.h"

#include "AbilitySystemComponent.h"
#include "Characters/PP_BossCharacter.h"
#include "Components/BoxComponent.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"
#include "GameFramework/Character.h"
#include "GameplayTags/PPTags.h"
#include "GameObjects/PP_BossArenaGate.h"

APP_BossArenaTrigger::APP_BossArenaTrigger()
{
	PrimaryActorTick.bCanEverTick = false;

	BoxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
	SetRootComponent(BoxComponent);
	BoxComponent->SetBoxExtent(FVector(1500.f, 1500.f, 300.f));
	BoxComponent->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	BoxComponent->SetGenerateOverlapEvents(true);
}

void APP_BossArenaTrigger::BeginPlay()
{
	Super::BeginPlay();

	ResolveReferencedActors();

	// 把门和 Boss 绑定起来，门据此只响应本场地 Boss 的死亡事件
	if (BossRef && GateRef && !GateRef->GetLinkedBoss())
	{
		GateRef->SetLinkedBoss(BossRef);
	}

	if (BoxComponent)
	{
		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
	}
}

void APP_BossArenaTrigger::ResolveReferencedActors()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// 未配置 BossRef 时就近自动查找；多份时告警并取最近的一个
	if (!BossRef)
	{
		int32 BossCount = 0;
		float BestDistSq = TNumericLimits<float>::Max();
		for (TActorIterator<APP_BossCharacter> It(World); It; ++It)
		{
			++BossCount;
			const float DistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				BossRef = *It;
			}
		}

		if (BossRef)
		{
			if (BossCount > 1)
			{
				UE_LOG(LogPP, Warning,
					TEXT("[BossArenaTrigger] %s 场景中有 %d 个 Boss，已就近自动绑定 %s；多 Boss 关卡请在蓝图显式配置 BossRef"),
					*GetName(), BossCount, *BossRef->GetName());
			}
			else
			{
				UE_LOG(LogPP, Log, TEXT("[BossArenaTrigger] %s 自动绑定 Boss %s"), *GetName(), *BossRef->GetName());
			}
		}
		else
		{
			UE_LOG(LogPP, Error,
				TEXT("[BossArenaTrigger] %s 未配置 BossRef 且场景中找不到 APP_BossCharacter，进入场地不会激活 Boss"),
				*GetName());
		}
	}

	// 未配置 GateRef 时就近自动查找
	if (!GateRef)
	{
		int32 GateCount = 0;
		float BestDistSq = TNumericLimits<float>::Max();
		for (TActorIterator<APP_BossArenaGate> It(World); It; ++It)
		{
			++GateCount;
			const float DistSq = FVector::DistSquared(GetActorLocation(), It->GetActorLocation());
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				GateRef = *It;
			}
		}

		if (GateRef)
		{
			if (GateCount > 1)
			{
				UE_LOG(LogPP, Warning,
					TEXT("[BossArenaTrigger] %s 场景中有 %d 个场地门，已就近自动绑定 %s；请在蓝图显式配置 GateRef"),
					*GetName(), GateCount, *GateRef->GetName());
			}
			else
			{
				UE_LOG(LogPP, Log, TEXT("[BossArenaTrigger] %s 自动绑定场地门 %s"), *GetName(), *GateRef->GetName());
			}
		}
		else
		{
			UE_LOG(LogPP, Warning,
				TEXT("[BossArenaTrigger] %s 未配置 GateRef 且场景中找不到 APP_BossArenaGate，进入场地不会关门"),
				*GetName());
		}
	}
}

void APP_BossArenaTrigger::OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (bArenaArmed || !OtherActor || OtherActor == this)
	{
		return;
	}

	// 只响应玩家角色
	if (!OtherActor->IsA<ACharacter>())
	{
		return;
	}

	bArenaArmed = true;

	// 给 Boss 加 ArenaArmed Tag（服务器权威，但 AddLooseGameplayTag 会自动 replicate）
	if (BossRef)
	{
		if (UAbilitySystemComponent* ASC = BossRef->GetAbilitySystemComponent())
		{
			ASC->AddLooseGameplayTag(PPTags::Status::BossArenaArmed);
		}
	}

	// 关闭场地门（阻挡碰撞）
	if (GateRef)
	{
		GateRef->SetActorEnableCollision(true);
	}

	OnArenaEntered();
}
