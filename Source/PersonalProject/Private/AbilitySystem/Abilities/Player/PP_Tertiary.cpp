// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/Player/PP_Tertiary.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameObjects/PP_FireballProjectile.h"
#include "GameplayTags/PPTags.h"

FGameplayTag UPP_Tertiary::GetEventTagToWait() const
{
	return PPTags::PPAbilities::Tertiary;
}

void UPP_Tertiary::OnAbilityEventReceived(FGameplayEventData Payload)
{
	// CommitAbility（蓝图忽略了返回值，这里加上标准的失败保护）
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

	// 在 Avatar 前方生成火球投射物
	const FVector SpawnLocation = AvatarActor->GetActorLocation() + AvatarActor->GetActorForwardVector() * ProjectileSpawnForwardOffset;

	FActorSpawnParameters SpawnParams;
	SpawnParams.Owner = AvatarActor; // 使投射物 GetOwner() 能取到施法者，供 Instigator / 击杀归属
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	GetWorld()->SpawnActor<APP_FireballProjectile>(ProjectileClass, SpawnLocation, AvatarActor->GetActorRotation(), SpawnParams);

	// 结束能力状态
	EndAbilityState(NAME_None);
}
