
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystem/Abilities/PP_GameplayAbility.h"
#include "PP_HitReact.generated.h"

/**
 * 
 */
UCLASS()
class PERSONALPROJECT_API UPP_HitReact : public UPP_GameplayAbility
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintCallable,Category="PP|Abilities")
	void CacheHitDirectionVectors(AActor* Instigator);
	
	UPROPERTY(BlueprintReadOnly,Category="PP|Abilities")
	FVector AvatarForward;
	
	UPROPERTY(BlueprintReadOnly,Category="PP|Abilities")
	FVector ToInstigator;
};
