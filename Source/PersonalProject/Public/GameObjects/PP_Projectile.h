
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

	UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="PP|Damage",meta=(ExposeOnSpawn,ClampMin = "0.0"))
	float Damage{-25.f};

	UFUNCTION(BlueprintImplementableEvent, Category = "PP|Projectile")
	void SpawnImpactEffects();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="PP|Damage", meta=(ExposeOnSpawn))
	TSubclassOf<UGameplayEffect> DamageEffect;

private:
	UPROPERTY(VisibleAnywhere,Category= "PP|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovement;
};
