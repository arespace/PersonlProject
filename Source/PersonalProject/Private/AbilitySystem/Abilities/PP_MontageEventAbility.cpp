// Chen Tianbao uses GAS to create online battle games

#include "AbilitySystem/Abilities/PP_MontageEventAbility.h"

#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "GameplayTags/PPTags.h"

UPP_MontageEventAbility::UPP_MontageEventAbility()
{
	// AbilityTask 需要通过 AddDynamic 绑定到本实例的委托，能力必须实例化
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPP_MontageEventAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent || BlockHitReactEffectClass == nullptr)
	{
		return;
	}

	// 1. 施加 Block Hit React GE（施法期间免于被击退/受击打断）
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(BlockHitReactEffectClass, BlockHitReactLevel, EffectContext);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2. 缓存激活参数，供蒙太奇结束回调里调用 EndAbility / 子类 CommitAbility
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;

	// 3. 播放施法蒙太奇
	UAnimMontage* MontageToPlay = GetMontageToPlay();
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
		bStopMontageWhenAbilityEnds,
		1.f,       // AnimRootMotionTranslationScale
		0.f        // StartTimeSeconds
	);

	// Completed / BlendOut / Interrupted / Cancelled 都结束能力（OnBlendedIn 不连）
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);

	MontageTask->ReadyForActivation();

	// 4. 等待蒙太奇里 AnimNotify 发出的技能事件
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		GetEventTagToWait(),
		nullptr, // OptionalExternalTarget
		false,   // OnlyTriggerOnce
		true     // OnlyMatchExact
	);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

UAnimMontage* UPP_MontageEventAbility::GetMontageToPlay()
{
	return AbilityMontage.Get();
}

void UPP_MontageEventAbility::OnAbilityMontageFinished()
{
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UPP_MontageEventAbility::OnGameplayEventReceived(FGameplayEventData Payload)
{
	OnAbilityEventReceived(Payload);
}

void UPP_MontageEventAbility::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 移除施法期间授予 PPTags.PPAbilities.BlockHitReact 的 GE（StacksToRemove = 1）
	BP_RemoveGameplayEffectFromOwnerWithGrantedTags(FGameplayTagContainer(PPTags::PPAbilities::BlockHitReact), 1);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
