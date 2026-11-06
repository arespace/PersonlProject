

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
	// 蒙太奇回调与兜底 Timer 可能都触发，只处理第一次
	if (bRespawnHandled) return;
	bRespawnHandled = true;

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

	// 角色侧复活计时（玩家，服务端权威）可能已先触发，避免重复复活，仅收尾结束能力
	if (Character && Character->IsAlive())
	{
		if (CachedActorInfo)
		{
			EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
		}
		return;
	}

	// 1. 移除授予 PPTags.Status.Dead 标签的 GE（对应 BP_RemoveGameplayEffectFromOwnerWithGrantedTags，1 层）
	BP_RemoveGameplayEffectFromOwnerWithGrantedTags(FGameplayTagContainer(PPTags::Status::Dead), 1);

	// 2. 发送 EndAttack 事件（对应 SendGameplayEventToActor）
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Avatar, PPTags::Events::Enemy::EndAttack, FGameplayEventData());

	// 3. 复活统一交给 ServerRespawn（仅服务端）：
	//    它会移除 Dead 标签、优先使用篝火记录的 PlayerState 重生点、用 TeleportPhysics 传送，
	//    并调用 HandleRespawn + 把落点同步给客户端。
	//    这里不再单独传送 PlayerStart / 直接调 HandleRespawn：
	//    - GA 的蒙太奇回调在客户端也会执行，客户端调用 HandleRespawn 会把已复活并开始移动的角色
	//      再次 StopMovement（日志里速度=375 被清零），表现就是复活后移动卡顿；
	//    - GA 路径传送到 PlayerStart 会绕过篝火重生点。
	if (Character && CachedActorInfo && CachedActorInfo->IsNetAuthority())
	{
		Character->RespawnCharacter();
	}

	// 5. 结束能力（对应 K2_EndAbility）
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}
