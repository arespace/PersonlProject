// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AbilitySystem/PP_AttributeSet.h"

#include "PP_AttributeWidget.generated.h"

/**
 * 
 */
UCLASS()
class PERSONALPROJECT_API UPP_AttributeWidget : public UUserWidget
{
	GENERATED_BODY()
	
public:

	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Crash|Attributes")
	FGameplayAttribute Attribute;
	
	UPROPERTY(EditAnywhere,BlueprintReadOnly, Category = "Crash|Attributes")
	FGameplayAttribute MaxAttribute;
	
	void OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair, UPP_AttributeSet* AttributeSet,float OldValue);
	bool MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair)const;
	
	UFUNCTION(BlueprintImplementableEvent,meta = (DisplayName = "On Attribute Change"))
	void BP_OnAttributeChage(float NewValue,float NewMaxValue,float OldValue);
	
	UPROPERTY(BlueprintReadOnly,Category = "Crash|Attributes")
	TWeakObjectPtr<AActor> AvatarActor;
	
};
