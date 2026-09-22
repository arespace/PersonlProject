// Chen Tianbao uses GAS to create online battle games


#include "GameObjects/PP_FireballProjectile.h"

#include "AbilitySystemComponent.h"
#include "Characters/PP_EnemyCharacter.h"
#include "GameplayTags/PPTags.h"
#include "Utils/PP_BlueprintLibrary.h"


APP_FireballProjectile::APP_FireballProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	ProjectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>("ProjectileMovement");

	bReplicates = true;
}

void APP_FireballProjectile::NotifyActorBeginOverlap(AActor* OtherActor)
{
	Super::NotifyActorBeginOverlap(OtherActor);

	APP_EnemyCharacter* Enemy = Cast<APP_EnemyCharacter>(OtherActor);
	if (!IsValid(Enemy)) return;
	if (!Enemy->IsAlive()) return;

	UAbilitySystemComponent* AbilitySystemComponent = Enemy->GetAbilitySystemComponent();
	if (!IsValid(AbilitySystemComponent) || !HasAuthority()) return;

	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = Enemy;
	// 对敌人：HitReact 事件用 Events.Enemy.HitReact（而非 Player），伤害 GE 用 SetByCaller.Player.Tertiary 幅度（对齐 Secondary 约定）
	UPP_BlueprintLibrary::SendDamageEventToPlayer(Enemy, DamageEffect, Payload, PPTags::SetByCaller::Player::Tertiary, Damage, PPTags::Events::Enemy::HitReact);

	SpawnImpactEffects();
	Destroy();
}
