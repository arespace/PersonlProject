// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Player/PP_Secondary.h"

#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Animation/AnimMontage.h"
#include "Camera/CameraShakeBase.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/PPTags.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Utils/PP_BlueprintLibrary.h"

UPP_Secondary::UPP_Secondary()
{
	// AbilityTask 需要通过 AddDynamic 绑定到本实例的委托，能力必须实例化
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPP_Secondary::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	UE_LOG(LogTemp, Error, TEXT("[UPP_Secondary] ActivateAbility 进入 — C++ 版在跑"));

	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent || BlockHitReactEffectClass == nullptr)
	{
		return;
	}

	// 1. Apply Block Hit React GE（对应 Sequence then_0）
	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(BlockHitReactEffectClass, 1.f, EffectContext);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());

	// 2. Play Montage（对应 Sequence then_1 → CreatePlayMontageAndWaitProxy，单一蒙太奇，无 FlipFlop）
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;

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
		false,     // bStopWhenAbilityEnds（蓝图 DefaultValue="false"）
		1.f,       // AnimRootMotionTranslationScale
		0.f        // StartTimeSeconds
	);

	// 对应蓝图：OnCompleted / OnBlendOut / OnInterrupted / OnCancelled 都连到 K2_EndAbility（OnBlendedIn 未连）
	MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnBlendOut.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);
	MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnAbilityMontageFinished);

	MontageTask->ReadyForActivation();

	// 3. Wait Gameplay Event（对应 Sequence then_2）：监听蒙太奇里 AnimNotify 发出的 PPTags.PPAbilities.Secondary
	UAbilityTask_WaitGameplayEvent* WaitEventTask = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(
		this,
		PPTags::PPAbilities::Secondary,
		nullptr, // OptionalExternalTarget
		false,   // OnlyTriggerOnce
		true     // OnlyMatchExact
	);
	WaitEventTask->EventReceived.AddDynamic(this, &ThisClass::OnGameplayEventReceived);
	WaitEventTask->ReadyForActivation();
}

void UPP_Secondary::OnGameplayEventReceived(FGameplayEventData Payload)
{
	UE_LOG(LogTemp, Error, TEXT("[UPP_Secondary] OnGameplayEventReceived 进入 — 收到 AnimNotify 事件"));

	// 对应蓝图：EventReceived → K2_CommitAbility（蓝图忽略了返回值，这里加上标准的失败保护）
	if (!CommitAbility(CachedHandle, CachedActorInfo, CachedActivationInfo))
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, true);
		return;
	}

	AActor* AvatarActor = GetAvatarActorFromActorInfo();
	if (!AvatarActor)
	{
		return;
	}

	// 1. 粒子（对应 Sequence_1 then_0 → SpawnEmitterAtLocation）
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, AvatarActor->GetActorLocation());
	}

	// 2. 镜头震动（对应 Sequence_1 then_1 → PlayWorldCameraShake）
	if (CameraShakeClass)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), CameraShakeClass, AvatarActor->GetActorLocation(), 1000.f, 1000.f, 1.f, false);
	}

	// 3. 命中检测 → 击退 → 伤害（对应 Sequence_1 then_2 → HitBoxOverlapTest → ApplyKnockback → SendDamageEventToPlayers）
	TArray<AActor*> ActorsHit = UPP_BlueprintLibrary::HitBoxOverlapTest(
		AvatarActor,
		HitBoxRadius,
		0.f, // HitBoxForwardOffset（蓝图未连，用默认 0）
		0.f, // HitBoxElevationOffset（蓝图未连，用默认 0）
		bDrawDebugs);

	TArray<AActor*> KnockedbackActors = UPP_BlueprintLibrary::ApplyKnockback(
		AvatarActor,
		ActorsHit,
		InnerRadius,
		HitBoxRadius, // OuterRadius 与 Hit Box Radius 共用同一变量
		LaunchForceMagnitude,
		RotationAngle,
		bDrawDebugs);

	FGameplayEventData DamagePayload;
	DamagePayload.Instigator = AvatarActor;
	UPP_BlueprintLibrary::SendDamageEventToPlayers(
		KnockedbackActors,
		DamageEffectClass,
		DamagePayload,
		PPTags::SetByCaller::Player::Secondary, // DataTag（SetByCaller 伤害）
		Damage,
		PPTags::None, // EventTagOverride
		nullptr);     // OptionalParticleSystem

	// 4. 结束能力状态（对应 Sequence_1 then_2 尾 → EndAbilityState）
	EndAbilityState(NAME_None);
}

void UPP_Secondary::OnAbilityMontageFinished()
{
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UPP_Secondary::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
	// 对应蓝图 K2_OnEndAbility：移除授予 PPTags.PPAbilities.BlockHitReact 的 GE（StacksToRemove = 1）
	BP_RemoveGameplayEffectFromOwnerWithGrantedTags(FGameplayTagContainer(PPTags::PPAbilities::BlockHitReact), 1);

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
