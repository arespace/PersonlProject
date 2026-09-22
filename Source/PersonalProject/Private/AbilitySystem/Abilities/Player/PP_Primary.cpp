// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Player/PP_Primary.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Engine/OverlapResult.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/PPTags.h"
#include "Utils/PP_BlueprintLibrary.h"

UPP_Primary::UPP_Primary()
{
	// FlipFlop 的交替状态要跨多次激活保留，能力必须用 InstancedPerActor
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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

void UPP_Primary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent || BlockHitReactEffectClass == nullptr)
	{
		return;
	}

	// 1. Apply Block Hit React GE（对应 Sequence then_0）
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(BlockHitReactEffectClass, 0.f, EffectContext);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2. Play Attack Montage（对应 Sequence then_1 → FlipFlop 交替左/右拳）
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;

	UAnimMontage* MontageToPlay = bUseLeftPunch ? LeftPunchMontage.Get() : RightPunchMontage.Get();
	bUseLeftPunch = !bUseLeftPunch; // 下次激活切另一侧

	if (!MontageToPlay)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		MontageToPlay,
		1.f,       // Rate
		NAME_None, // StartSection
		true,      // bStopWhenAbilityEnds
		1.f,       // AnimRootMotionTranslationScale
		0.f        // StartTimeSeconds
	);

	// 对应蓝图：OnCompleted / OnBlendOut / OnInterrupted / OnCancelled 都连到 K2_EndAbility（OnBlendedIn 未连）
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAttackMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnAttackMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAttackMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAttackMontageFinished);

	MontageTask->ReadyForActivation();

	// 3. Wait Gameplay Event（对应 Sequence then_2）：监听攻击蒙太奇里 AnimNotify 发出的 PPTags.Events.Player.Primary
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PPTags::Events::Player::Primary,
		nullptr, // OptionalExternalTarget
		false,   // OnlyTriggerOnce
		true     // OnlyMatchExact
	);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void UPP_Primary::OnAttackMontageFinished()
{
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UPP_Primary::OnGameplayEventReceived(FGameplayEventData Payload)
{
	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AvatarActor || !AbilitySystemComponent || DamageEffectClass == nullptr)
	{
		return;
	}

	// 1. HitBox 检测（对应 PP_BlueprintLibrary::HitBoxOverlapTest）
	TArray<AActor*> ActorsHit = UPP_BlueprintLibrary::HitBoxOverlapTest(
		AvatarActor,
		HitBoxRadius,
		HitBoxForwardOffset,
		HitBoxElevationOffset,
		bDrawDebugs);

	// 2. 通知被命中的敌人（对应 SendHitReactEventToActors）
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

void UPP_Primary::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 对应蓝图 K2_OnEndAbility：移除授予 PPTags.PPAbilities.BlockHitReact 的 GE（StacksToRemove = 1）
	BP_RemoveGameplayEffectFromOwnerWithGrantedTags(FGameplayTagContainer(PPTags::PPAbilities::BlockHitReact), 1);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
