// Chen Tianbao uses GAS to create online battle games


#include "HUD/PP_AttributeWidget.h"

void UPP_AttributeWidget::OnAttributeChange(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair,
	UPP_AttributeSet* AttributeSet,float OldValue)
{
	const float AttributeValue = Pair.Key.GetNumericValue(AttributeSet);
	const float MaxAttributeValue = Pair.Value.GetNumericValue(AttributeSet);
	
	BP_OnAttributeChage(AttributeValue,MaxAttributeValue, OldValue);
	
}

bool UPP_AttributeWidget::MatchesAttributes(const TTuple<FGameplayAttribute, FGameplayAttribute>& Pair) const
{
	return Attribute == Pair.Key && MaxAttribute == Pair.Value;
}
