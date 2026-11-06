
#include "AI/UPP_AIPerceptionComponent.h"

#include "AIController.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Characters/PP_BaseCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"
#include "TimerManager.h"

UPP_AIPerceptionComponent::UPP_AIPerceptionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UPP_AIPerceptionComponent::BeginPlay()
{
	Super::BeginPlay();
	StartSearching();
}

void UPP_AIPerceptionComponent::StartSearching()
{
	AAIController* AIC = GetOwningAIController();
	if (!AIC)
	{
		UE_LOG(LogPP, Warning, TEXT("[AIPerception] 未找到所属 AIController，搜索未启动"));
		return;
	}

	// 服务端权威搜敌（客户端不跑）
	if (!GetOwner() || !GetOwner()->HasAuthority() || !GetWorld())
	{
		return;
	}

	GetWorld()->GetTimerManager().ClearTimer(SearchTimerHandle);
	GetWorld()->GetTimerManager().SetTimer(
		SearchTimerHandle,
		this,
		&ThisClass::Search,
		SearchInterval,
		true);
}

void UPP_AIPerceptionComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorld()->GetTimerManager().ClearTimer(SearchTimerHandle);
	Super::EndPlay(EndPlayReason);
}

AAIController* UPP_AIPerceptionComponent::GetOwningAIController() const
{
	return Cast<AAIController>(GetOwner());
}

FVector UPP_AIPerceptionComponent::GetPerceiverLocation() const
{
	if (AAIController* AIC = GetOwningAIController())
	{
		if (APawn* Pawn = AIC->GetPawn())
		{
			return Pawn->GetActorLocation();
		}
	}
	return FVector::ZeroVector;
}

void UPP_AIPerceptionComponent::ClearTarget()
{
	CurrentTarget.Reset();
	bLeashed = false;
	LastTargetValidTime = -1.f;

	if (AAIController* AIC = GetOwningAIController())
	{
		AIC->ClearFocus(EAIFocusPriority::Gameplay);
		AIC->StopMovement();
	}
}

bool UPP_AIPerceptionComponent::CanAttackNow(float AcceptanceDistance) const
{
	if (!CurrentTarget.IsValid()) return false;
	const float Dist = FVector::Dist(GetPerceiverLocation(), CurrentTarget->GetActorLocation());
	return Dist <= AcceptanceDistance;
}

void UPP_AIPerceptionComponent::Search()
{
	AAIController* AIC = GetOwningAIController();
	if (!AIC) return;

	APawn* Pawn = AIC->GetPawn();
	if (!Pawn) return;

	const FVector PerceiverLocation = Pawn->GetActorLocation();

	// === 第一步：先检查当前目标是否仍有效（粘性 + leash）===
	bool bCurrentStillValid = false;
	if (CurrentTarget.IsValid())
	{
		const FVector CurrentLoc = CurrentTarget->GetActorLocation();
		const float Dist = FVector::Dist(PerceiverLocation, CurrentLoc);

		if (Dist > LeashRange)
		{
			bLeashed = true;
			CurrentTarget.Reset();
			UE_LOG(LogPP, Log, TEXT("[AIPerception] Leash 触发：距离=%.0f > %.0f"), Dist, LeashRange);
		}
		else
		{
			bLeashed = false;
			if (Dist <= SearchRadius)
			{
				const float HeightDiff = FMath::Abs(CurrentLoc.Z - PerceiverLocation.Z);
				if (HeightDiff <= MaxHeightDiff)
				{
					FHitResult Hit;
					FCollisionQueryParams Params(SCENE_QUERY_STAT(AIPerceptionSearch), false, Pawn);
					Params.AddIgnoredActor(CurrentTarget.Get());
					const bool bCanSee = !GetWorld()->LineTraceSingleByChannel(
						Hit, PerceiverLocation, CurrentLoc, ECC_Visibility, Params);
					if (bCanSee)
					{
						bCurrentStillValid = true;
						LastTargetValidTime = GetWorld()->GetTimeSeconds();
					}
				}
			}
			if (!bCurrentStillValid)
			{
				const float Now = GetWorld()->GetTimeSeconds();
				if (Now - LastTargetValidTime <= TargetLostGraceTime)
				{
					bCurrentStillValid = true;
				}
			}
		}
	}

	if (bCurrentStillValid)
	{
		return;
	}

	// === 第二步：扫所有玩家角色找新目标 ===
	CurrentTarget.Reset();

	UWorld* World = GetWorld();
	if (!World) return;

	for (TActorIterator<APP_BaseCharacter> It(World); It; ++It)
	{
		APP_BaseCharacter* Candidate = *It;
		if (!IsValid(Candidate) || !Candidate->IsAlive()) continue;

		if (!Candidate->ActorHasTag(PPActorTags::Player)) continue;

		const FVector CandidateLoc = Candidate->GetActorLocation();
		const float Dist = FVector::Dist(PerceiverLocation, CandidateLoc);
		if (Dist > SearchRadius) continue;

		const float HeightDiff = FMath::Abs(CandidateLoc.Z - PerceiverLocation.Z);
		if (HeightDiff > MaxHeightDiff) continue;

		FHitResult Hit;
		FCollisionQueryParams Params(SCENE_QUERY_STAT(AIPerceptionSearch), false, Pawn);
		Params.AddIgnoredActor(Candidate);
		if (GetWorld()->LineTraceSingleByChannel(
			Hit, PerceiverLocation, CandidateLoc, ECC_Visibility, Params))
		{
			continue;
		}

		if (!CurrentTarget.IsValid() || Dist < FVector::Dist(PerceiverLocation, CurrentTarget->GetActorLocation()))
		{
			CurrentTarget = Candidate;
			LastTargetValidTime = GetWorld()->GetTimeSeconds();
		}
	}

	if (CurrentTarget.IsValid())
	{
		AIC->SetFocus(CurrentTarget.Get());
	}
}
