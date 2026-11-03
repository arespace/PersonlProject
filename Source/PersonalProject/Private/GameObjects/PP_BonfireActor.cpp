// Chen Tianbao uses GAS to create online battle games

#include "GameObjects/PP_BonfireActor.h"

#include "Characters/PP_BaseCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Framework/PP_Log.h"
#include "Game/PP_CrashGameMode.h"
#include "Game/PP_EnemyRespawnManager.h"
#include "GameFramework/PlayerController.h"
#include "Player/PP_PlayerController.h"
#include "Player/PP_PlayerState.h"
#include "UObject/ConstructorHelpers.h"

APP_BonfireActor::APP_BonfireActor()
{
	PrimaryActorTick.bCanEverTick = false;
	bReplicates = true;
	SetReplicateMovement(false);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Mesh->SetCollisionObjectType(ECC_WorldStatic);
	Mesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	// 玩家按 E 时从相机前方做 Visibility 线检测，篝火网格必须能被 trace 命中
	Mesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	RootComponent = Mesh;

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderMesh.Succeeded())
	{
		Mesh->SetStaticMesh(CylinderMesh.Object);
		Mesh->SetRelativeScale3D(FVector(0.6f, 0.6f, 0.8f));
	}

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(RootComponent);
	Light->SetRelativeLocation(FVector(0.f, 0.f, 60.f));
	Light->SetLightColor(FLinearColor(1.f, 0.55f, 0.2f));
	Light->SetIntensity(5000.f);
	Light->SetAttenuationRadius(600.f);
	Light->SetCastShadows(false);

	RespawnPoint = CreateDefaultSubobject<USceneComponent>(TEXT("RespawnPoint"));
	RespawnPoint->SetupAttachment(RootComponent);
	// 胶囊中心高度 ≈ 96（玩家 InitCapsuleSize 42/96），放在篝火侧前方并略高于地面，
	// 避免复活时和篝火模型/地面穿插
	RespawnPoint->SetRelativeLocation(FVector(120.f, 0.f, 110.f));

	// 独立交互碰撞盒：只响应 Visibility，保证 E 键射线始终能命中，不阻挡玩家移动
	InteractionCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionCollision"));
	InteractionCollision->SetupAttachment(RootComponent);
	InteractionCollision->SetBoxExtent(FVector(90.f, 90.f, 110.f));
	InteractionCollision->SetRelativeLocation(FVector(0.f, 0.f, 100.f));
	InteractionCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionCollision->SetCollisionObjectType(ECC_WorldStatic);
	InteractionCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionCollision->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	InteractionCollision->SetGenerateOverlapEvents(false);
	InteractionCollision->SetHiddenInGame(true);
}

void APP_BonfireActor::Interact(APlayerController* Interactor)
{
	if (!Interactor || !CanInteract(Interactor))
	{
		return;
	}

	// 篝火是关卡 Actor，不属于任何客户端，直接在本 Actor 上发 Server RPC 会被丢弃
	//（没有 owning connection）。改由拥有者 PlayerController 转发 Server RPC。
	if (APP_PlayerController* PC = Cast<APP_PlayerController>(Interactor))
	{
		PC->ServerInteractBonfire(this);
	}
}

bool APP_BonfireActor::CanInteract(APlayerController* Interactor) const
{
	const APawn* Pawn = Interactor ? Interactor->GetPawn() : nullptr;
	const APP_BaseCharacter* Character = Cast<APP_BaseCharacter>(Pawn);
	return IsValid(Character) && Character->IsAlive();
}

void APP_BonfireActor::Rest(APlayerController* Interactor)
{
	if (!HasAuthority() || !Interactor)
	{
		return;
	}

	APP_BaseCharacter* Character = Cast<APP_BaseCharacter>(Interactor->GetPawn());
	APP_PlayerState* PlayerState = Cast<APP_PlayerState>(Interactor->PlayerState);
	if (!IsValid(Character) || !IsValid(PlayerState) || !Character->IsAlive())
	{
		return;
	}

	// 服务端再校验一次距离，防止客户端伪造交互
	const float Distance = FVector::Dist(Character->GetActorLocation(), GetActorLocation());
	if (Distance > InteractDistance)
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Bonfire] 休息失败：距离过远 %.1f > %.1f 玩家=%s"),
			Distance, InteractDistance, *GetNameSafe(PlayerState));
		return;
	}

	FTransform CheckpointTransform = RespawnPoint ? RespawnPoint->GetComponentTransform() : GetActorTransform();

	// 用向下射线把重生点贴到地面上，并按玩家胶囊半高抬高。
	// 否则 RespawnPoint 的默认高度可能让胶囊底部嵌进地板，复活后移动会被反复解穿透，表现为卡顿。
	FVector CheckpointLocation = CheckpointTransform.GetLocation();
	FVector TraceStart = CheckpointLocation + FVector(0.f, 0.f, 120.f);
	FVector TraceEnd = CheckpointLocation - FVector(0.f, 0.f, 600.f);

	FHitResult FloorHit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(BonfireRespawnGround), false, Character);
	Params.AddIgnoredActor(this);

	if (GetWorld()->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_Visibility, Params))
	{
		const float CapsuleHalfHeight = Character->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		CheckpointLocation.Z = FloorHit.Location.Z + CapsuleHalfHeight + 2.f;
		CheckpointTransform.SetLocation(CheckpointLocation);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Bonfire] 重生点下方未找到地面，保持原始高度 %.0f"), CheckpointLocation.Z);
	}

	PlayerState->SetCheckpoint(CheckpointTransform);

	// 回满生命/法力/耐力（ResetAttributesEffect 在角色蓝图里配置）
	Character->ResetAttributes();

	if (APP_PlayerController* PC = Cast<APP_PlayerController>(Interactor))
	{
		PC->Client_NotifyMessage(TEXT("已在篝火休息：属性已回满，重生点已记录"));
	}

	UE_LOG(LogPP, Log, TEXT("[PP_Bonfire] %s 在篝火 %s 休息：重生点=(%.0f, %.0f, %.0f) 朝向=%.0f"),
		*GetNameSafe(PlayerState), *GetName(),
		CheckpointTransform.GetLocation().X, CheckpointTransform.GetLocation().Y, CheckpointTransform.GetLocation().Z,
		CheckpointTransform.Rotator().Yaw);

	// === 休息成功 → 重置半径内的敌人 ===
	if (UWorld* World = GetWorld())
	{
		if (AGameModeBase* GM = World->GetAuthGameMode())
		{
			if (UPP_EnemyRespawnManager* Manager = GM->FindComponentByClass<UPP_EnemyRespawnManager>())
			{
				const int32 ResetCount = Manager->ResetEnemiesInRadius(GetActorLocation(), ResetRadius);
				UE_LOG(LogPP, Log, TEXT("[PP_Bonfire] 休息触发敌人重置：Radius=%.0f 重置 %d 个敌人"),
					ResetRadius, ResetCount);
			}
			else
			{
				UE_LOG(LogPP, Warning, TEXT("[PP_Bonfire] 休息触发敌人重置：未找到 UPP_EnemyRespawnManager（需要在 GameMode 蓝图子类里 Add Component）"));
			}
		}
	}
}
