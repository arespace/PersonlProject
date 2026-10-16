// Chen Tianbao uses GAS to create online battle games


#include "Characters/PP_BaseCharacter.h"

#include "GameplayAbilitySpec.h"
#include "AbilitySystemComponent.h"
#include "Net/UnrealNetwork.h"

namespace PersonalTags
{
	const FName Player = FName("Player");
}

APP_BaseCharacter::APP_BaseCharacter()
{
	PrimaryActorTick.bCanEverTick = false;
	
	//Tick and refresh bone transforms whether rendered or not - for bone updates on a dedicated server
	GetMesh()->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
}

void APP_BaseCharacter::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME(ThisClass, bAlive);
}

UAbilitySystemComponent* APP_BaseCharacter::GetAbilitySystemComponent() const
{
	return nullptr;
}


void APP_BaseCharacter::GiveStartupAbilities()
{
	if (!IsValid(GetAbilitySystemComponent())) return;
	for (const auto& Ability	: StartupAbilities)
	{
		FGameplayAbilitySpec AbilitySpec = FGameplayAbilitySpec(Ability);
		GetAbilitySystemComponent()->GiveAbility(AbilitySpec);
	}
}

void APP_BaseCharacter::InitializeAttributes() const
{
	checkf(IsValid(InitializeAttributesEffect) , TEXT("InitializeAttributesEffect not set."));
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(InitializeAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void APP_BaseCharacter::OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	if (AttributeChangeData.NewValue <= 0.f)
	{
		HandleDeath();
	}
}

void APP_BaseCharacter::HandleDeath()
{
	bAlive = false;
}

void APP_BaseCharacter::HandleRespawn()
{
	bAlive = true;
}

void APP_BaseCharacter::ResetAttributes()
{
	checkf(IsValid(ResetAttributesEffect) , TEXT("ResetAttributesEffect not set."));
	if (!IsValid(GetAbilitySystemComponent())) return;
	
	FGameplayEffectContextHandle ContextHandle = GetAbilitySystemComponent()->MakeEffectContext();
	FGameplayEffectSpecHandle SpecHandle = GetAbilitySystemComponent()->MakeOutgoingSpec(ResetAttributesEffect, 1.f, ContextHandle);
	GetAbilitySystemComponent()->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

