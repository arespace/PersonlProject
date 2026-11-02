// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/PP_GameplayAbility.h"

#include "AbilitySystem/PP_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayTags/PPTags.h"

void UPP_GameplayAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	if (bDrawDebugs)
	{
		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, FString::Printf(TEXT("%s Activate: "), *GetName()));
	}

	// 体力在激活时统一扣除：部分技能（如 Fifth）不调用 CommitAbility，
	// 放在 ActivateAbility 才能保证所有配置了 StaminaCost 的技能都生效。
	if (StaminaCost > 0.f)
	{
		ApplyStaminaCost(ActorInfo);
	}
}

bool UPP_GameplayAbility::CheckCost(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CheckCost(Handle, ActorInfo, OptionalRelevantTags))
	{
		return false;
	}

	if (StaminaCost <= 0.f || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return true;
	}

	// 没有 PP_AttributeSet 的角色（敌人）不做体力拦截
	const UPP_AttributeSet* AttributeSet = ActorInfo->AbilitySystemComponent->GetSet<UPP_AttributeSet>();
	if (!AttributeSet)
	{
		return true;
	}

	return AttributeSet->GetStamina() >= StaminaCost;
}

void UPP_GameplayAbility::ApplyStaminaCost(const FGameplayAbilityActorInfo* ActorInfo) const
{
	if (!StaminaCostEffect || !ActorInfo || !ActorInfo->AbilitySystemComponent.IsValid())
	{
		return;
	}

	UAbilitySystemComponent* AbilitySystemComponent = ActorInfo->AbilitySystemComponent.Get();

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(ActorInfo->AvatarActor.Get());

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaCostEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	SpecHandle.Data->SetSetByCallerMagnitude(PPTags::SetByCaller::StaminaCost, -StaminaCost);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}
