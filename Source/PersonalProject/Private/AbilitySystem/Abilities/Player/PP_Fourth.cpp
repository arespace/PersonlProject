// Chen Tianbao uses GAS to create online battle games

#include "AbilitySystem/Abilities/Player/PP_Fourth.h"
#include "GameObjects/PP_Projectile.h"
#include "Engine/World.h"
#include "Framework/PP_Log.h"

void UPP_Fourth::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogPP, Error, TEXT("[策略实验] ActivateAbility 执行端: %s"),
		(ActorInfo && ActorInfo->IsNetAuthority()) ? TEXT("Server") : TEXT("Client"));


	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	if (IsValid(ProjectileClass) && ActorInfo && ActorInfo->AvatarActor.IsValid())
	{
		AActor* Avatar = ActorInfo->AvatarActor.Get();
		const FVector SpawnLoc = Avatar->GetActorLocation() + Avatar->GetActorForwardVector() * 100.f;
		GetWorld()->SpawnActor<APP_Projectile>(ProjectileClass, SpawnLoc, Avatar->GetActorRotation());
	}

	EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}
