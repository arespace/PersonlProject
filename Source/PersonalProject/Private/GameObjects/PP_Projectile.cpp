// Chen Tianbao uses GAS to create online battle games


#include "GameObjects/PP_Projectile.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Characters/PP_PlayerCharacter.h"
#include "GameplayTags/PPTags.h"
#include "Utils/PP_BlueprintLibrary.h"


APP_Projectile::APP_Projectile()
{
	PrimaryActorTick.bCanEverTick = false;
	
	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");
	
	bReplicates = true;
}

void APP_Projectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);
	APP_PlayerCharacter* PlayerCharacter = Cast<APP_PlayerCharacter>(OtherActor);
	if (!IsValid(PlayerCharacter)) return;
	if (!PlayerCharacter->IsAlive()) return;
	UAbilitySystemComponent* AbilitySystemComponent = PlayerCharacter->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent) || !HasAuthority()) return;
	
	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = PlayerCharacter;
	UPP_BlueprintLibrary::SendDamageEventToPlayer(PlayerCharacter,DamageEffect,Payload,PPTags::SetByCaller::Projectile,Damage,PPTags::None); 
	
	SpawnImpactEffects();
	Destroy();
}


