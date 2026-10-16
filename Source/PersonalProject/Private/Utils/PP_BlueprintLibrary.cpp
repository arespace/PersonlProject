// Chen Tianbao uses GAS to create online battle games


#include "Utils/PP_BlueprintLibrary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "GameplayEffect.h"
#include "Characters/PP_BaseCharacter.h"
#include "Characters/PP_EnemyCharacter.h"
#include "Engine/OverlapResult.h"
#include "GameplayTags/PPTags.h"
#include "Kismet/GameplayStatics.h"

EHitDirection UPP_BlueprintLibrary::GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator)
{
	const float Dot = FVector::DotProduct(TargetForward, ToInstigator);
	
	if (Dot < -0.5f )
	{
			return EHitDirection::Back;
	}
	if (Dot > 0.5f )
	{
		//Either Left or Right
		const FVector Cross = FVector::CrossProduct(TargetForward, ToInstigator);
		if (Cross.Z < 0.f )
		{
			return EHitDirection::Left;
		}
		return EHitDirection::Right;
	}
	return EHitDirection::Forward;
}

FName UPP_BlueprintLibrary::GetHitDirectionName(const EHitDirection& HitDirection)
{
	switch (HitDirection)
	{
	case EHitDirection::Left: return FName("Left");
	case EHitDirection::Right: return FName("Right");
	case EHitDirection::Forward: return FName("Forward");
	case EHitDirection::Back: return FName("Back");
	default: return FName("None");
	}
}

FClosestActorWithTagResult UPP_BlueprintLibrary::FindClosestActorWithTag( UObject* WorldContextObject,
	const FVector& Origin, const FName& Tag,float SearchRange)
{
	TArray<AActor*> ActorsWithTag;
	UGameplayStatics::GetAllActorsWithTag(WorldContextObject, Tag, ActorsWithTag);
	
	float ClosestDistance = TNumericLimits<float>::Max();
	AActor* ClosestActor = nullptr;
	
	for (AActor* Actor : ActorsWithTag)
	{
		if (!IsValid(Actor)) continue;
		APP_BaseCharacter* BaseCharacter = Cast<APP_BaseCharacter>(Actor);
		if (!IsValid(BaseCharacter) || !BaseCharacter->IsAlive()) continue;
		
		const float Distance = FVector::Dist(Origin,Actor->GetActorLocation());
		if (APP_BaseCharacter* SearchingCharacter = Cast<APP_BaseCharacter>(WorldContextObject); IsValid(SearchingCharacter))
		{
			if (Distance > SearchingCharacter->SearchRange)continue;
		}
		if (Distance < ClosestDistance)
		{
			ClosestDistance = Distance;
			ClosestActor = Actor;
		}
	}
	
	FClosestActorWithTagResult Result;
	Result.Actor = ClosestActor;
	Result.Distance = ClosestDistance;
	
	return Result;
	
}

void UPP_BlueprintLibrary::SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect,
	FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSyetem)
{
	APP_BaseCharacter* PlayerCharacter = Cast<APP_BaseCharacter>(Target);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;
	
	FGameplayTag EventTag;
	if (!EventTagOverride.MatchesTagExact(PPTags::None))
	{
		EventTag = EventTagOverride;
	}
	else
	{
		UPP_AttributeSet* AttributeSet = Cast<UPP_AttributeSet>(PlayerCharacter->GetAttributeSet());
		if (!IsValid(AttributeSet)) return;

		const bool bLethal = AttributeSet->GetHealth() - Damage <= 0.f;
		EventTag = bLethal ? PPTags::Events::Player::Death : PPTags::Events::Player::HitReact;
	}
	
	Payload.OptionalObject = OptionalParticleSyetem;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PlayerCharacter,EventTag,Payload);

	UAbilitySystemComponent* TargetASC = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(TargetASC)) return;

	FGameplayEffectContextHandle ContextHandle = TargetASC->MakeEffectContext();
	// 将攻击者信息写入 GE Context + Target Actor，使 HandleDeath 的 GetInstigatorController() 能获取到击杀者
	if (Payload.Instigator)
	{
		ContextHandle.AddSourceObject(Payload.Instigator);
		PlayerCharacter->SetInstigator(const_cast<APawn*>(Cast<APawn>(Payload.Instigator)));
	}
	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffect, 1.f	, ContextHandle);

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle,DataTag,-Damage);

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

}

void UPP_BlueprintLibrary::SendDamageEventToPlayers(TArray<AActor*> Targets,
	const TSubclassOf<UGameplayEffect>& DamageEffect, FGameplayEventData& Payload, const FGameplayTag& DataTag,
	float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem)
{
	for (AActor* Target : Targets)
	{
		SendDamageEventToPlayer(Target, DamageEffect, Payload, DataTag, Damage, EventTagOverride, OptionalParticleSystem);
	}
}

void UPP_BlueprintLibrary::ApplyDamageEffectFromCauser(AActor* Causer, AActor* Target,
	const TSubclassOf<UGameplayEffect>& DamageEffect, const FGameplayTag& CoefficientTag, float Coefficient)
{
	if (!IsValid(Causer) || !IsValid(Target) || !DamageEffect)
	{
		return;
	}

	UAbilitySystemComponent* CauserASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Causer);
	UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!IsValid(CauserASC) || !IsValid(TargetASC))
	{
		return;
	}

	// 关键：spec 必须建在【施法者】ASC 上。MakeEffectContext() 内部会执行
	// AddInstigator(GetOwnerActor(), GetAvatarActor())，所以 Source 才解析成施法者；
	// 若建在 TargetASC 上，ExecCalc 捕获 Source 的攻击力会读到受击者自己。
	FGameplayEffectContextHandle ContextHandle = CauserASC->MakeEffectContext();
	ContextHandle.AddSourceObject(Causer);

	// 让受击者的 GetInstigatorController() 能取到击杀者（供 HandleDeath 的任务击杀通知使用）
	if (APawn* CauserPawn = Cast<APawn>(Causer))
	{
		if (APP_BaseCharacter* TargetCharacter = Cast<APP_BaseCharacter>(Target))
		{
			TargetCharacter->SetInstigator(CauserPawn);
		}
	}

	FGameplayEffectSpecHandle SpecHandle = CauserASC->MakeOutgoingSpec(DamageEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	UAbilitySystemBlueprintLibrary::AssignTagSetByCallerMagnitude(SpecHandle, CoefficientTag, Coefficient);

	CauserASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
}

TArray<AActor*> UPP_BlueprintLibrary::HitBoxOverlapTest(AActor* AvatarActor, float HitBoxRadius,float HitBoxForwardOffset,float HitBoxElevationOffset,bool bDrawDebugs)
{
	if (!IsValid(AvatarActor)) return TArray<AActor*>();

	
	//Ensure that the overlap test ignores the Avatar Actor
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(AvatarActor);
	
	FCollisionResponseParams ResponseParams;
	ResponseParams.CollisionResponse.SetAllChannels(ECR_Ignore);
	ResponseParams.CollisionResponse.SetResponse(ECC_Pawn,ECR_Block);
	
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(HitBoxRadius);

	const FVector Forward = AvatarActor->GetActorForwardVector() * HitBoxForwardOffset;
	const FVector HitBoxLocation = AvatarActor->GetActorLocation() + Forward + FVector(0,0,HitBoxElevationOffset);
	
	UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return TArray<AActor*>();
	World->OverlapMultiByChannel(OverlapResults,HitBoxLocation,FQuat::Identity,ECC_Visibility,Sphere,QueryParams,ResponseParams);

	TArray<AActor*> ActorsHit;
	for (const FOverlapResult& Result : OverlapResults)
	{
		APP_BaseCharacter* BaseCharacter = Cast<APP_BaseCharacter>(Result.GetActor());
		if (!IsValid(BaseCharacter)) continue;
		if (!BaseCharacter->IsAlive()) continue;
		ActorsHit.AddUnique(BaseCharacter);
		
	}
	
	if (bDrawDebugs)
	{
		DrawHitBoxOverlapDebugs(AvatarActor,OverlapResults,HitBoxLocation,HitBoxRadius);
	}
	
	return ActorsHit;
}

void UPP_BlueprintLibrary::DrawHitBoxOverlapDebugs(const UObject* WorldContextObject,const TArray<FOverlapResult>& OverlapResults,
	const FVector& HitBoxLocation,float HitBoxRadius)
{
	UWorld* World = GEngine->GetWorldFromContextObject(WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
	if (!IsValid(World)) return;
	DrawDebugSphere(World,HitBoxLocation,HitBoxRadius,16,FColor::Red,false,3.f);
	for (const FOverlapResult& Result:OverlapResults)
	{
		
		if (IsValid(Result.GetActor()))
		{
			FVector DebugLocation = Result.GetActor()->GetActorLocation();
			DebugLocation.Z += 100.f;
			DrawDebugSphere(World,DebugLocation,30.f,10,FColor::Green,false,3.f);
				
		}
	}
}

TArray<AActor*> UPP_BlueprintLibrary::ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors,
	float InnerRadius, float OuterRadius, float LaunchForceMagnitude, float RotationAngle, bool bDrawDebugs)
{
	for (AActor* HitActor : HitActors)
	{
		ACharacter* HitCharacter = Cast<ACharacter>(HitActor);
		if (!IsValid(HitCharacter) || !IsValid(AvatarActor)) return TArray<AActor*>();

		const FVector HitCharacterLocation = HitCharacter->GetActorLocation();
		const FVector AvatarLocation = AvatarActor->GetActorLocation();

		const FVector ToHitActor = HitCharacterLocation - AvatarLocation;
		const float Distance = FVector::Dist(AvatarLocation, HitCharacterLocation);

		float LaunchForce = 0.f;
		if (Distance > OuterRadius) continue;
		if (Distance <= InnerRadius)
		{
			LaunchForce = LaunchForceMagnitude;
		}
		else
		{
			const FVector2D FalloffRange(InnerRadius, OuterRadius); // input range
			const FVector2D LaunchForceRange(LaunchForceMagnitude, 0.f); // output range
			LaunchForce = FMath::GetMappedRangeValueClamped(FalloffRange, LaunchForceRange, Distance);
		}
		if (bDrawDebugs) GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Red, FString::Printf(TEXT("LaunchForce: %f"), LaunchForce));

		FVector KnockbackForce = ToHitActor.GetSafeNormal();
		KnockbackForce.Z = 0.f;

		const FVector Right = KnockbackForce.RotateAngleAxis(90.f, FVector::UpVector);
		KnockbackForce = KnockbackForce.RotateAngleAxis(-RotationAngle, Right) * LaunchForce;

		if (bDrawDebugs)
		{
			UWorld* World = GEngine->GetWorldFromContextObject(AvatarActor, EGetWorldErrorMode::LogAndReturnNull);
			DrawDebugDirectionalArrow(World, HitCharacterLocation, HitCharacterLocation + KnockbackForce, 100.f, FColor::Green, false, 3.f);
		}

		if (APP_EnemyCharacter* EnemyCharacter = Cast<APP_EnemyCharacter>(HitCharacter); IsValid(EnemyCharacter))
		{
			EnemyCharacter->StopMovementUntilLanded();
		} 

		HitCharacter->LaunchCharacter(KnockbackForce, true, true);
	}
	return HitActors;
}
