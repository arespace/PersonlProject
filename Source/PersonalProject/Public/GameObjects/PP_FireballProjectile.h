// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PP_FireballProjectile.generated.h"

class UProjectileMovementComponent;
class UGameplayEffect;

/**
 * 火球术（UPP_Tertiary）的投射物：玩家 → 敌人。
 * 与 APP_Projectile（敌人 → 玩家）相反，命中时对 APP_EnemyCharacter 造成伤害。
 */
UCLASS()
class PERSONALPROJECT_API APP_FireballProjectile : public AActor
{
	GENERATED_BODY()

public:
	APP_FireballProjectile();
	virtual void NotifyActorBeginOverlap(AActor* OtherActor) override;

	// 伤害数值（SetByCaller 幅度，负值表示扣血，与 APP_Projectile 约定一致）
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crash|Damage", meta = (ExposeOnSpawn, ClampMin = "0.0"))
	float Damage{-25.f};

	UFUNCTION(BlueprintImplementableEvent, Category = "Crash|Projectile")
	void SpawnImpactEffects();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Crash|Damage", meta = (ExposeOnSpawn))
	TSubclassOf<UGameplayEffect> DamageEffect;

private:
	UPROPERTY(VisibleAnywhere, Category = "Crash|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
};
