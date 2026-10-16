// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Player/PP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameplayTags/PPTags.h"
#include "Utils/PP_BlueprintLibrary.h"

UPP_Primary::UPP_Primary()
{
	// 蓝图原版：BlockHitReact GE 等级 0、能力结束时主动停蒙太奇
	BlockHitReactLevel = 0.f;
	bStopMontageWhenAbilityEnds = true;
}

void UPP_Primary::SendHitReactEventToActors(const TArray<AActor*> ActorsHit)
{
	for (AActor* HitActor : ActorsHit)
	{
		FGameplayEventData Payload;
		Payload.Instigator = GetAvatarActorFromActorInfo();
		UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(HitActor,PPTags::Events::Enemy::HitReact,Payload);
	}
}

UAnimMontage* UPP_Primary::GetMontageToPlay()
{
	// FlipFlop 交替左/右拳，下次激活切另一侧
	UAnimMontage* MontageToPlay = bUseLeftPunch ? LeftPunchMontage.Get() : RightPunchMontage.Get();
	bUseLeftPunch = !bUseLeftPunch;
	return MontageToPlay;
}

FGameplayTag UPP_Primary::GetEventTagToWait() const
{
	return PPTags::Events::Player::Primary;
}

void UPP_Primary::OnAbilityEventReceived(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AvatarActor || !AbilitySystemComponent || DamageEffectClass == nullptr)
	{
		return;
	}

	// 1. HitBox 检测
	TArray<AActor*> ActorsHit = UPP_BlueprintLibrary::HitBoxOverlapTest(
		AvatarActor,
		HitBoxRadius,
		HitBoxForwardOffset,
		HitBoxElevationOffset,
		bDrawDebugs);

	// 2. 通知被命中的敌人
	SendHitReactEventToActors(ActorsHit);

	// 3. 对每个命中的敌人施加伤害 GE（对应 ForEachLoop → BP_ApplyGameplayEffectSpecToTarget）
	for (AActor* HitActor : ActorsHit)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(HitActor);
		if (!TargetASC)
		{
			continue;
		}

		FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
		EffectContext.AddSourceObject(AvatarActor);
		const FGameplayEffectSpecHandle DamageSpec = AbilitySystemComponent->MakeOutgoingSpec(
			DamageEffectClass, static_cast<float>(GetAbilityLevel()), EffectContext);
		AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*DamageSpec.Data.Get(), TargetASC);
	}
}
