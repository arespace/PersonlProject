// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Abilities/PP_Death.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Animation/AnimMontage.h"
#include "Characters/PP_BaseCharacter.h"
#include "Characters/PP_EnemyCharacter.h"
#include "Engine/World.h"
#include "GameFramework/GameModeBase.h"
#include "GameplayTagContainer.h"
#include "GameplayTags/PPTags.h"

UPP_Death::UPP_Death()
{
	// AbilityTask 需要通过 AddDynamic 绑定到本实例的委托，能力必须实例化
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UPP_Death::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;

	// 1. 播放死亡蒙太奇（对应 CreatePlayMontageAndWaitProxy，MontageToPlay = DeathMontage）
	if (DeathMontage)
	{
		UAbilityTask_PlayMontageAndWait* MontageTask = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
			this,
			NAME_None,
			DeathMontage,
			1.f,       // Rate
			NAME_None, // StartSection
			true,      // bStopWhenAbilityEnds
			1.f,       // AnimRootMotionTranslationScale
			0.f        // StartTimeSeconds
		);

		// 对应蓝图：OnCompleted / OnInterrupted / OnCancelled 都连到敌人 Cast（OnBlendOut / OnBlendedIn 未连）
		MontageTask->OnCompleted.AddDynamic(this, &ThisClass::OnDeathMontageFinished);
		MontageTask->OnInterrupted.AddDynamic(this, &ThisClass::OnDeathMontageFinished);
		MontageTask->OnCancelled.AddDynamic(this, &ThisClass::OnDeathMontageFinished);

		MontageTask->ReadyForActivation();
	}

	// 2. 施加 GE_Death（对应蒙太奇任务节点的 "then" 立即输出 → BP_ApplyGameplayEffectSpecToSelf）
	ApplyDeathEffect();
}

void UPP_Death::ApplyDeathEffect()
{
	UAbilitySystemComponent* AbilitySystemComponent = GetAbilitySystemComponentFromActorInfo();
	if (!AbilitySystemComponent || DeathEffectClass == nullptr)
	{
		return;
	}

	FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
	EffectContext.AddSourceObject(GetAvatarActorFromActorInfo());
	const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DeathEffectClass, 1.f, EffectContext);
	AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
}

void UPP_Death::OnDeathMontageFinished()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	// 对应蓝图：Cast 头像到 BP_PP_EnemyBase
	if (APP_EnemyCharacter* Enemy = Cast<APP_EnemyCharacter>(Avatar))
	{
		if (Enemy->bShouldRespawn)
		{
			Respawn();
		}
		else
		{
			// 销毁尸体（对应 SetLifeSpan(1.0) → SetActorHiddenInGame(true)）
			Avatar->SetLifeSpan(1.f);
			Avatar->SetActorHiddenInGame(true);
		}
	}
	else
	{
		// 非敌人（玩家）：直接复活
		Respawn();
	}
}

void UPP_Death::Respawn()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		return;
	}

	APP_BaseCharacter* Character = Cast<APP_BaseCharacter>(Avatar);

	// 1. 移除授予 PPTags.Status.Dead 标签的 GE（对应 BP_RemoveGameplayEffectFromOwnerWithGrantedTags，1 层）
	BP_RemoveGameplayEffectFromOwnerWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead), 1);

	// 2. 发送 EndAttack 事件（对应 SendGameplayEventToActor）
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, PPTags::Events::Enemy::EndAttack, FGameplayEventData());

	// 3. 仅服务端：重置属性 + 传送到 PlayerStart（对应 K2_HasAuthority 分支）
	if (Character && CachedActorInfo && CachedActorInfo->IsNetAuthority())
	{
		Character->ResetAttributes();

		if (UWorld* World = GetWorld())
		{
			if (AGameModeBase* GameMode = World->GetAuthGameMode())
			{
				if (AActor* PlayerStart = GameMode->FindPlayerStart(Character->GetController()))
				{
					Character->SetActorLocation(PlayerStart->GetActorLocation(), false);
				}
			}
		}
	}

	// 4. 复活角色（对应 HandleRespawn）
	if (Character)
	{
		Character->HandleRespawn();
	}

	// 5. 结束能力（对应 K2_EndAbility）
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}
