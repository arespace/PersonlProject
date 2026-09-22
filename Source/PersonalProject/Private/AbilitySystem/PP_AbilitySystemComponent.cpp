// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/PP_AbilitySystemComponent.h"

#include "GameplayTags/PPTags.h"


void UPP_AbilitySystemComponent::OnGiveAbility(FGameplayAbilitySpec& AbilitySpec)
{
	Super::OnGiveAbility(AbilitySpec);
	
	HandlAutoActivatedAbility(AbilitySpec);	
	
}

void UPP_AbilitySystemComponent::OnRep_ActivateAbilities()
{
	Super::OnRep_ActivateAbilities();
	
	FScopedAbilityListLock ActiveScopeLock(*this);
	for ( const FGameplayAbilitySpec& AbilitySpec : GetActivatableAbilities())
	{
		HandlAutoActivatedAbility(AbilitySpec);	
		
	}
}

void UPP_AbilitySystemComponent::SetAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	if (IsValid(GetAvatarActor()) && !GetAvatarActor()->HasAuthority()) return;
	
	
	
	if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(AbilityClass))
	{
		AbilitySpec->Level = Level;
		MarkAbilitySpecDirty(*AbilitySpec);
	}
}

void UPP_AbilitySystemComponent::AddToAbilityLevel(TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	if (IsValid(GetAvatarActor()) && !GetAvatarActor()->HasAuthority()) return;
	
	
	
	if (FGameplayAbilitySpec* AbilitySpec = FindAbilitySpecFromClass(AbilityClass))
	{
		AbilitySpec->Level += Level;
		MarkAbilitySpecDirty(*AbilitySpec);
	}
}

void UPP_AbilitySystemComponent::HandlAutoActivatedAbility(const FGameplayAbilitySpec& AbilitySpec)
{
	if (!IsValid(AbilitySpec.Ability)) return;
	
	for (const FGameplayTag& Tag : AbilitySpec.Ability->GetAssetTags())
	{
		if (Tag.MatchesTagExact(PPTags::PPAbilities::ActivateOnGiven))
		{
			TryActivateAbility(AbilitySpec.Handle);
			return;
		}
	}
}

float UPP_AbilitySystemComponent::GetCooldownRemainingForTag(const FGameplayTag& CooldownTag) const
{
	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(CooldownTag);

	// UE5.7: GetActiveEffectsWithAllTags 返回 TArray
	TArray<FActiveGameplayEffectHandle> ActiveHandles = GetActiveEffectsWithAllTags(CooldownTags);

	float MaxRemaining = 0.f;
	for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
	{
		if (const FActiveGameplayEffect* ActiveEffect = GetActiveGameplayEffect(Handle))
		{
			const float TimeRemaining = ActiveEffect->GetTimeRemaining(GetWorld()->GetTimeSeconds());
			if (TimeRemaining > MaxRemaining)
			{
				MaxRemaining = TimeRemaining;
			}
		}
	}

	return MaxRemaining;
}

float UPP_AbilitySystemComponent::GetCooldownDurationForTag(const FGameplayTag& CooldownTag) const
{
	FGameplayTagContainer CooldownTags;
	CooldownTags.AddTag(CooldownTag);

	// UE5.7: GetActiveEffectsWithAllTags 返回 TArray
	TArray<FActiveGameplayEffectHandle> ActiveHandles = GetActiveEffectsWithAllTags(CooldownTags);

	float MaxDuration = 0.f;
	for (const FActiveGameplayEffectHandle& Handle : ActiveHandles)
	{
		if (const FActiveGameplayEffect* ActiveEffect = GetActiveGameplayEffect(Handle))
		{
			const float TotalDuration = ActiveEffect->Spec.GetDuration();
			if (TotalDuration > MaxDuration)
			{
				MaxDuration = TotalDuration;
			}
		}
	}

	return MaxDuration;
}
