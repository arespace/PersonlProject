// Chen Tianbao uses GAS to create online battle games

#include "GameObjects/PP_BossArenaTrigger.h"

#include "AbilitySystemComponent.h"
#include "Characters/PP_BossCharacter.h"
#include "Components/BoxComponent.h"
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

	if (BoxComponent)
	{
		BoxComponent->OnComponentBeginOverlap.AddDynamic(this, &ThisClass::OnOverlapBegin);
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
