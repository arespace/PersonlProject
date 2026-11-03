// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/PP_GameplayAbility.h"

#include "AbilitySystem/PP_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "Framework/PP_Log.h"
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
		// 排错：记录每份能力的实例/本地角色/扣除前后的体力。
		// 同一技能被重复授予时一次按键会出现多条该日志，体力也会被扣多次。
		const UAbilitySystemComponent* AbilitySystemComponent = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
		const UPP_AttributeSet* AttributeSet = AbilitySystemComponent ? AbilitySystemComponent->GetSet<UPP_AttributeSet>() : nullptr;

		UE_LOG(LogPP, Warning,
			TEXT("[PP_Skill] 扣除体力: %s 实例=%s 本机=%d LocalRole=%d 消耗=%.1f 体力前=%.1f/%.1f"),
			*GetName(), *GetNameSafe(GetCurrentAbilitySpec() ? GetCurrentAbilitySpec()->GetPrimaryInstance() : nullptr),
			ActorInfo && ActorInfo->IsLocallyControlled() ? 1 : 0,
			AbilitySystemComponent ? static_cast<int32>(AbilitySystemComponent->GetOwnerRole()) : -1,
			StaminaCost,
			AttributeSet ? AttributeSet->GetStamina() : -1.f,
			AttributeSet ? AttributeSet->GetMaxStamina() : -1.f);

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
