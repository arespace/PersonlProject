// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/AbilityTasks/PP_WaitGameplayEvent.h"

UPP_WaitGameplayEvent* UPP_WaitGameplayEvent::WaitGameplayEventToActorProxy(AActor* TargetActor, FGameplayTag EventTag, bool OnlyTriggerOnce, bool OnlyMatchExact)
{
	UPP_WaitGameplayEvent* MyObj = NewObject<UPP_WaitGameplayEvent>();
	MyObj->SetAbilityActor(TargetActor);
	MyObj->Tag = EventTag;
	MyObj->OnlyTriggerOnce = OnlyTriggerOnce;
	MyObj->OnlyMatchExact = OnlyMatchExact;
	return MyObj;
}

void UPP_WaitGameplayEvent::StartActivation()
{
    Activate();
}
