// Chen Tianbao uses GAS to create online battle games


#include "GameObjects/PP_FireballProjectile.h"

#include "AbilitySystemBlueprintLibrary.h"
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
	if (!HasAuthority()) return;

	// 1. 受击表现：用 Events.Enemy.HitReact 触发敌人受击反应。
	//    敌人死亡不在这里判定 —— 由 OnHealthChanged → HandleDeath 负责，与伤害数值解耦。
	FGameplayEventData Payload;
	Payload.Instigator = GetOwner();
	Payload.Target = Enemy;
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Enemy, PPTags::Events::Enemy::HitReact, Payload);

	// 2. 伤害结算：以施法者身份发放 GE，伤害由 UPP_DamageExecutionCalculation 计算
	//    （最终伤害 = 施法者攻击力 × DamageCoefficient − 敌人防御）
	UPP_BlueprintLibrary::ApplyDamageEffectFromCauser(GetOwner(), Enemy, DamageEffect,
		PPTags::SetByCaller::Player::Tertiary, DamageCoefficient);

	SpawnImpactEffects();
	Destroy();
}
