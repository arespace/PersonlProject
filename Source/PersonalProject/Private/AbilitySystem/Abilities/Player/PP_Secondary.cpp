// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Player/PP_Secondary.h"

#include "AbilitySystemComponent.h"
#include "Camera/CameraShakeBase.h"
#include "GameplayTags/PPTags.h"
#include "Kismet/GameplayStatics.h"
#include "Particles/ParticleSystem.h"
#include "Utils/PP_BlueprintLibrary.h"

FGameplayTag UPP_Secondary::GetEventTagToWait() const
{
	return PPTags::PPAbilities::Secondary;
}

void UPP_Secondary::OnAbilityEventReceived(FGameplayEventData Payload)
{
	// CommitAbility（蓝图忽略了返回值，这里加上标准的失败保护）
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

	// 1. 粒子（对应 SpawnEmitterAtLocation）
	if (ImpactParticle)
	{
		UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), ImpactParticle, AvatarActor->GetActorLocation());
	}

	// 2. 镜头震动（对应 PlayWorldCameraShake）
	if (CameraShakeClass)
	{
		UGameplayStatics::PlayWorldCameraShake(GetWorld(), CameraShakeClass, AvatarActor->GetActorLocation(), 1000.f, 1000.f, 1.f, false);
	}

	// 3. 命中检测 → 击退 → 伤害
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

	// 4. 结束能力状态
	EndAbilityState(NAME_None);
}
