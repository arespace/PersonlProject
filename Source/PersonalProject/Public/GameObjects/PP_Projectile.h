// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "PP_Projectile.generated.h"

class UProjectileMovementComponent;
class UGameplayEffect;

UCLASS()
class PERSONALPROJECT_API APP_Projectile : public AActor
{
	GENERATED_BODY()

public:
	APP_Projectile();
	virtual void NotifyActorBeginOverlap(AActor* OtherActor);

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Crash|Damage",meta=(ExposeOnSpawn,ClampMin = "0.0"))
	float Damage{-25.f};

	UFUNCTION(BlueprintImplementableEvent, Category = "Crash|Projectile")
	void SpawnImpactEffects();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Crash|Damage", meta=(ExposeOnSpawn))
	TSubclassOf<UGameplayEffect> DamageEffect;

private:
	UPROPERTY(VisibleAnywhere,Category= "Crash|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
};
