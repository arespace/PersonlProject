// Chen Tianbao uses GAS to create online battle games


#include "PersonalProject/Public/AbilitySystem/Abilities/PP_GameplayAbility.h"
			void UPP_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
           	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
           	const FGameplayEventData* TriggerEventData)
			{
				Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
				
				if (bDrawDebugs)
				{
					GEngine->AddOnScreenDebugMessage(-1,3.f,FColor::Cyan,FString::Printf(TEXT("%s Activate: "),*GetName()));
				}
			}

