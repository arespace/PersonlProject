// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Player/PP_Tertiary.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameObjects/PP_FireballProjectile.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/PPTags.h"

UPP_Tertiary::UPP_Tertiary()
{
	// AbilityTask 需要通过 AddDynamic 绑定到本实例的委托，能力必须实例化
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPP_Tertiary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent || BlockHitReactEffectClass == nullptr)
	{
		return;
	}

	// 1. 施加 Block Hit React GE（对应 Sequence then_0，与 Secondary 一致：施法期间免于被击退/受击打断）
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(BlockHitReactEffectClass, 1.f, EffectContext);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2. 缓存激活参数，供蒙太奇结束回调里调用 EndAbility / CommitAbility
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;

	// 3. 播放施法蒙太奇（对应 CreatePlayMontageAndWaitProxy）
	if (!AbilityMontage)
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
		return;
	}

	UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
		this,
		NAME_None,
		AbilityMontage,
		1.f,       // Rate
		NAME_None, // StartSection
		false,     // bStopWhenAbilityEnds（对应蓝图 DefaultValue="false"）
		1.f,       // AnimRootMotionTranslationScale
		0.f        // StartTimeSeconds
	);

	// 对应蓝图：OnCompleted / OnBlendOut / OnInterrupted / OnCancelled 都连到 K2_EndAbility（OnBlendedIn 未连）
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);

	MontageTask->ReadyForActivation();

	// 4. Wait Gameplay Event（对应 Sequence then_2）：监听施法蒙太奇里 AnimNotify 发出的 PPTags.PPAbilities.Tertiary
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PPTags::PPAbilities::Tertiary,
		nullptr, // OptionalExternalTarget
		false,   // OnlyTriggerOnce
		true     // OnlyMatchExact
	);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void UPP_Tertiary::OnGameplayEventReceived(FGameplayEventData Payload)
{
	// 对应蓝图：EventReceived → K2_CommitAbility（蓝图忽略了返回值，这里加上标准的失败保护）
	if (!CommitAbility(CachedHandle, CachedActorInfo, CachedActivationInfo))
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor || !IsValid(ProjectileClass))
	{
		return;
	}

	// 3. 在 Avatar 前方生成火球投射物（对应 SpawnActor）
	const FVector SpawnLocation = AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * ProjectileSpawnForwardOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor; // 使投射物 GetOwner() 能取到施法者，供 Instigator / 击杀归属
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<APP_FireballProjectile>(ProjectileClass, SpawnLocation, AvatarActor->GetActorRotation(), SpawnParams);

	// 4. 结束能力状态（对应 Sequence 尾 → EndAbilityState）
	EndAbilityState(NAME_None);
}

void UPP_Tertiary::OnAbilityMontageFinished()
{
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UPP_Tertiary::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 对应蓝图 K2_OnEndAbility：移除授予 PPTags.PPAbilities.BlockHitReact 的 GE（StacksToRemove = 1，与 Secondary 一致）
	BP_RemoveGameplayEffectFromOwnerWithGrantedTags(FGameplayTagContainer(PPTags::PPAbilities::BlockHitReact), 1);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
