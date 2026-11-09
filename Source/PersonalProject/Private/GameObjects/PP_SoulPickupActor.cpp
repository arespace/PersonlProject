// Chen Tianbao uses GAS to create online battle games

#include "GameObjects/PP_SoulPickupActor.h"

#include "Characters/PP_BaseCharacter.h"
#include "Components/PointLightComponent.h"
#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/PP_Log.h"
#include "GameFramework/Pawn.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Net/UnrealNetwork.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "UObject/ConstructorHelpers.h"

APP_SoulPickupActor::APP_SoulPickupActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	PickupRange = CreateDefaultSubobject<USphereComponent>(TEXT("PickupRange"));
	PickupRange->InitSphereRadius(140.f);
	PickupRange->SetCollisionProfileName(TEXT("OverlapAllDynamic"));
	PickupRange->SetGenerateOverlapEvents(true);
	RootComponent = PickupRange;

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(RootComponent);
	Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Mesh->SetCastShadow(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		Mesh->SetStaticMesh(SphereMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.55f));
	}

	Glow = CreateDefaultSubobject<UPointLightComponent>(TEXT("Glow"));
	Glow->SetupAttachment(RootComponent);
	Glow->SetRelativeLocation(FVector(0.f, 0.f, 40.f));
	Glow->SetLightColor(FLinearColor(0.35f, 0.55f, 1.f));
	Glow->SetIntensity(3000.f);
	Glow->SetAttenuationRadius(250.f);
	Glow->SetCastShadows(false);
}

void APP_SoulPickupActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_SoulPickupActor, Amount);
	DOREPLIFETIME(APP_SoulPickupActor, OwnerState);
}

void APP_SoulPickupActor::BeginPlay()
{
	Super::BeginPlay();

	// Overlap 事件按规范在 BeginPlay 中绑定
	if (PickupRange)
	{
		PickupRange->OnComponentBeginOverlap.AddDynamic(this, &APP_SoulPickupActor::OnPickupRangeBeginOverlap);
	}
}

void APP_SoulPickupActor::InitializeSoul(APP_PlayerState* InOwnerState, int32 InAmount)
{
	if (!HasAuthority())
	{
		return;
	}

	OwnerState = InOwnerState;
	Amount = InAmount;
	ForceNetUpdate();

	UE_LOG(LogPP, Log, TEXT("[PP_SoulFlow] 生成魂: %s 拥有者=%s 金币=%d"),
		*GetName(), *GetNameSafe(InOwnerState), Amount);
}

void APP_SoulPickupActor::OnPickupRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!HasAuthority() || Amount <= 0 || !IsValid(OwnerState))
	{
		return;
	}

	APawn* Pawn = Cast<APawn>(OtherActor);
	if (!Pawn)
	{
		return;
	}

	// 只有掉落魂的本人能拾回
	APP_PlayerState* PawnState = Pawn->GetPlayerState<APP_PlayerState>();
	if (PawnState != OwnerState)
	{
		return;
	}

	APP_BaseCharacter* Character = Cast<APP_BaseCharacter>(Pawn);
	if (!Character || !Character->IsAlive())
	{
		return;
	}

	UPP_InventoryComponent* Inventory = PawnState->GetInventoryComponent();
	if (!IsValid(Inventory))
	{
		return;
	}

	Inventory->AddGold(Amount);

	if (APP_PlayerController* PC = Cast<APP_PlayerController>(Pawn->GetController()))
	{
		PC->Client_NotifyMessage(FString::Printf(TEXT("拾回魂：+%d 金币"), Amount));
	}

	UE_LOG(LogPP, Log, TEXT("[PP_SoulFlow] 拾回魂: %s 玩家=%s 金币=%d"),
		*GetName(), *GetNameSafe(PawnState), Amount);

	Amount = 0;
	Destroy();
}
