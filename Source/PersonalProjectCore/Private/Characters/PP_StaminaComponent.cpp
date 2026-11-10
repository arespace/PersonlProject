#include "Characters/PP_StaminaComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Characters/PP_BaseCharacter.h"
#include "Framework/PP_Log.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameplayEffect.h"
#include "GameplayTags/PPTags.h"
#include "Utility/AlsGameplayTags.h"

UPP_StaminaComponent::UPP_StaminaComponent()
{
	// 由角色 Tick 手动驱动，保持与迁移前一致的帧内更新顺序
	PrimaryComponentTick.bCanEverTick = false;
}

APP_BaseCharacter* UPP_StaminaComponent::GetOwnerCharacter() const
{
	return Cast<APP_BaseCharacter>(GetOwner());
}

void UPP_StaminaComponent::BindToAbilitySystem()
{
	APP_BaseCharacter* Character = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	// 先 RemoveAll 再 Add：监听服务器上 PossessedBy 与 OnRep_PlayerState 都会执行，
	// 直接 AddUObject 会重复绑定，导致一次消耗触发两次回复延迟
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPP_AttributeSet::GetStaminaAttribute()).RemoveAll(this);
	AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UPP_AttributeSet::GetStaminaAttribute()).AddUObject(this, &ThisClass::OnStaminaChanged);
}

void UPP_StaminaComponent::ApplyPassiveEffects()
{
	APP_BaseCharacter* Character = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(AbilitySystemComponent))
	{
		return;
	}

	// 两个常驻 Infinite 周期 GE：是否真正生效由 OngoingTagRequirements 决定
	const TSubclassOf<UGameplayEffect> PassiveEffects[] = {StaminaRegenEffect, StaminaSprintDrainEffect};
	for (const TSubclassOf<UGameplayEffect>& EffectClass : PassiveEffects)
	{
		if (!EffectClass)
		{
			continue;
		}

		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(Character);
		FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(EffectClass, 1.f, ContextHandle);
		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
}

void UPP_StaminaComponent::UpdateStamina()
{
	UpdateSprintStateTag();
	UpdateRollStaminaCost();
}

bool UPP_StaminaComponent::HasEnoughStamina(const float Amount) const
{
	// 取不到 ASC / 属性集时不拦截，保持迁移前的行为（预检失败即放行）
	const APP_BaseCharacter* Character = GetOwnerCharacter();
	const UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;
	if (Amount <= 0.f || !IsValid(AbilitySystemComponent))
	{
		return true;
	}

	return AbilitySystemComponent->GetNumericAttribute(UPP_AttributeSet::GetStaminaAttribute()) >= Amount;
}

void UPP_StaminaComponent::ApplyStaminaCost(const float Amount)
{
	APP_BaseCharacter* Character = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;
	if (Amount <= 0.f || !StaminaCostEffect || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(Character);
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaCostEffect, 1.f, ContextHandle);
	if (!SpecHandle.IsValid())
	{
		return;
	}

	// SetByCaller 传负值，Instant GE 用加法扣减
	SpecHandle.Data->SetSetByCallerMagnitude(PPTags::SetByCaller::StaminaCost, -Amount);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UPP_StaminaComponent::OnStaminaChanged(const FOnAttributeChangeData& AttributeChangeData)
{
	// 只处理"消耗"（耐力下降）；回复不触发延迟
	if (AttributeChangeData.NewValue >= AttributeChangeData.OldValue)
	{
		return;
	}

	APP_BaseCharacter* Character = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;
	if (!IsValid(Character) || !Character->IsAlive() || !IsValid(AbilitySystemComponent) || !StaminaRegenDelayEffect)
	{
		return;
	}

	// 每次消耗都刷新延迟窗口（GE 使用 Aggregate + Refresh 叠加策略，不会叠加堆栈）
	FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
	ContextHandle.AddSourceObject(Character);
	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(StaminaRegenDelayEffect, 1.f, ContextHandle);
	if (SpecHandle.IsValid())
	{
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
}

void UPP_StaminaComponent::UpdateSprintStateTag()
{
	APP_BaseCharacter* Character = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;

	// 耐力消耗由服务端权威 GE 执行，状态 tag 只在服务端维护
	if (!IsValid(Character) || !Character->HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		return;
	}

	// "真的在冲刺"才算：ALS 的 Gait 在速度为 0 时仍可能保持 Sprinting，
	// 必须叠加"在地面 + 无翻滚/倒地等动作"判断，避免站着不动或翻滚时掉耐力。
	const UCharacterMovementComponent* Movement = Character->GetCharacterMovement();
	const bool bNowSprinting =
		Character->IsAlive()
		&& Character->GetGait() == AlsGaitTags::Sprinting
		&& !Character->GetLocomotionAction().IsValid()
		&& IsValid(Movement) && Movement->IsMovingOnGround();

	const bool bHasSprintingTag = AbilitySystemComponent->HasMatchingGameplayTag(PPTags::Status::Sprinting);
	if (bNowSprinting && !bHasSprintingTag)
	{
		AbilitySystemComponent->AddLooseGameplayTag(PPTags::Status::Sprinting);
	}
	else if (!bNowSprinting && bHasSprintingTag)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(PPTags::Status::Sprinting);
	}
}

void UPP_StaminaComponent::UpdateRollStaminaCost()
{
	APP_BaseCharacter* Character = GetOwnerCharacter();
	UAbilitySystemComponent* AbilitySystemComponent = IsValid(Character) ? Character->GetAbilitySystemComponent() : nullptr;

	// 翻滚是 ALS 动作状态，只在服务端扣体力
	if (!IsValid(Character) || !Character->HasAuthority() || !IsValid(AbilitySystemComponent))
	{
		bWasRolling = false;
		return;
	}

	const bool bRolling = Character->GetLocomotionAction() == AlsLocomotionActionTags::Rolling;
	if (bRolling && !bWasRolling)
	{
		ApplyStaminaCost(RollStaminaCost);
	}

	bWasRolling = bRolling;
}
