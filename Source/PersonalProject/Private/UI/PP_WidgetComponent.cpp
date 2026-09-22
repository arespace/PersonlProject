// Chen Tianbao uses GAS to create online battle games


#include "UI/PP_WidgetComponent.h"

#include "AbilitySystem/PP_AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Blueprint/WidgetTree.h"
#include "Characters/PP_BaseCharacter.h"
#include "UI/PP_AttributeWidget.h"

void UPP_WidgetComponent::BeginPlay()
{
	Super::BeginPlay();
	InitAbilitySystemData();

	if (!IsASCInitialized())
	{
		CrashCharacter->OnASCInitialized.AddDynamic(this, &ThisClass::OnASCInitialized);
		return;
	}
	InitializeAttributeDelegate();
}

void UPP_WidgetComponent::InitAbilitySystemData()
{
	CrashCharacter = Cast<APP_BaseCharacter>(GetOwner());
	AttributeSet = Cast<UPP_AttributeSet>(CrashCharacter->GetAttributeSet());
	AbilitySystemComponent = Cast<UPP_AbilitySystemComponent>(CrashCharacter->GetAbilitySystemComponent());
}

bool UPP_WidgetComponent::IsASCInitialized() const
{
	return AbilitySystemComponent.IsValid() && AttributeSet.IsValid();
}

void UPP_WidgetComponent::InitializeAttributeDelegate()
{
	if (!AttributeSet->bAttributesInitialized)
	{
		AttributeSet->OnAttributesInitialized.AddDynamic(this, &ThisClass::BindToAttributeChanges);
	}
	else
	{
		BindToAttributeChanges();
	}
}



void UPP_WidgetComponent::OnASCInitialized(UAbilitySystemComponent* ASC, UAttributeSet* AS)
{
	AbilitySystemComponent = Cast<UPP_AbilitySystemComponent>(ASC);
	AttributeSet = Cast<UPP_AttributeSet>(AS);
	
	if (!IsASCInitialized()) return;
	InitializeAttributeDelegate();
}

void UPP_WidgetComponent::BindToAttributeChanges()
{
	for (const TTuple<FGameplayAttribute,FGameplayAttribute>& Pair:AttributeMap)
	{
		
		BindWidgetToAttributeChanges(GetUserWidgetObject(),Pair); // for checking the owned widget object
		
		GetUserWidgetObject()->WidgetTree->ForEachWidget([this, &Pair](UWidget* ChildWidget)
		{
			BindWidgetToAttributeChanges(ChildWidget,Pair);
		});
		
	}
}

void UPP_WidgetComponent::BindWidgetToAttributeChanges(UWidget* WidgetObject,
	const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	//Check the user widget owned by this component, see if it is a PP_AttributeWidget , handle it if so.
	UPP_AttributeWidget* AttributeWidget = Cast<UPP_AttributeWidget>(WidgetObject);
	if (!IsValid(AttributeWidget)) return; // We only care about PP Attribute Widget
	if (!AttributeWidget->MatchesAttributes(Pair)) return; // Only subscribe for matching Attributes
	AttributeWidget->AvatarActor = CrashCharacter;
		
	AttributeWidget->OnAttributeChange(Pair,AttributeSet.Get(),0.f);
		
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(Pair.Key).AddLambda([this , AttributeWidget , &Pair] (const FOnAttributeChangeData& AttributeChangeData)
	{
		AttributeWidget->OnAttributeChange(Pair,AttributeSet.Get(),AttributeChangeData.OldValue); //for changes during the game
	});
		
}