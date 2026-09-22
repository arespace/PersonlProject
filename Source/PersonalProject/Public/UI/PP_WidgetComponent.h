// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Components/WidgetComponent.h"
#include "PP_WidgetComponent.generated.h"

class UAbilitySystemComponent;
class UPP_AttributeSet;
class APP_BaseCharacter;
class UPP_AbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_WidgetComponent : public UWidgetComponent
{
	GENERATED_BODY()
	

protected:

	virtual void BeginPlay() override;
	
	UPROPERTY(EditAnywhere)
	TMap<FGameplayAttribute,FGameplayAttribute> AttributeMap;

private:
	TWeakObjectPtr<APP_BaseCharacter> CrashCharacter;
	TWeakObjectPtr<UPP_AbilitySystemComponent> AbilitySystemComponent;
	TWeakObjectPtr<UPP_AttributeSet> AttributeSet;
	
	void InitAbilitySystemData();
	bool IsASCInitialized() const;
	void InitializeAttributeDelegate();
	void BindWidgetToAttributeChanges(UWidget* WidgetObject, const TTuple<FGameplayAttribute,FGameplayAttribute>& Pair) const;
	
	UFUNCTION()
	void OnASCInitialized(UAbilitySystemComponent* ASC , UAttributeSet* AS);
	
	UFUNCTION()
	void BindToAttributeChanges();
	
};
