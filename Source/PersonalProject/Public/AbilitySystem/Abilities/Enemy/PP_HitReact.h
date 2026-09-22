// Chen Tianbao uses GAS to create online battle games

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
	
	UFUNCTION(BlueprintCallable,Category="Crash|Abilities")
	void CacheHitDirectionVectors(AActor* Instigator);
	
	UPROPERTY(BlueprintReadOnly,Category="Crash|Abilities")
	FVector AvatarForward;
	
	UPROPERTY(BlueprintReadOnly,Category="Crash|Abilities")
	FVector ToInstigator;
};
