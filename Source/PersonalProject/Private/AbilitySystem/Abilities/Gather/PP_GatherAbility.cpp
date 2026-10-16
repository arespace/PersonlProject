// 采集系统 — GAS采集能力 实现

#include "AbilitySystem/Abilities/Gather/PP_GatherAbility.h"

#include "AbilitySystemComponent.h"
#include "Gather/PP_GatherableActor.h"
#include "GameFramework/Character.h"
#include "TimerManager.h"

UPP_GatherAbility::UPP_GatherAbility()
{
	// 能力实例化策略：每次激活使用独立实例
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

	// 网络执行策略：仅在服务器执行（客户端通过Tag感知状态）
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;

	// 配置Tag：激活时自动施加 State.Gathering，作为采集状态标记
	// 在蓝图/默认对象中设定：
	//   AbilityTags = Ability.Action.Gather
	//   ActivationOwnedTags = State.Gathering
	//   BlockAbilitiesWithTag = State.Gathering（采集时阻止其他能力）

	CachedActorInfo = nullptr;
}

void UPP_GatherAbility::SetGatherTarget(APP_GatherableActor* InTarget)
{
	GatherTarget = InTarget;
}

bool UPP_GatherAbility::CanActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
	{
		return false;
	}

	// 必须已设置采集目标
	if (!GatherTarget || !GatherTarget->IsAvailable())
	{
		return false;
	}

	return true;
}

void UPP_GatherAbility::ActivateAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	// 缓存参数
	CachedHandle = Handle;
	CachedActorInfo = ActorInfo;
	CachedActivationInfo = ActivationInfo;

	UE_LOG(LogTemp, Log, TEXT("[GatherAbility] ActivateAbility: target=%s gatherTime=%.1f"),
		GatherTarget ? *GatherTarget->GetName() : TEXT("null"),
		GatherTarget ? GatherTarget->GetGatherTime() : 0.0f);

	// 通知采集物开始采集
	if (GatherTarget)
	{
		AActor* AvatarActor = ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr;
		GatherTarget->Server_StartGather(AvatarActor);
	}

	// 播放采集蒙太奇（可选）
	if (GatherMontage)
	{
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo ? ActorInfo->AvatarActor.Get() : nullptr))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Play(GatherMontage);
			}
		}
	}

	// 注册 State.Gathering Tag 变化监听（当Tag被外部移除时打断采集）
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayTag GatheringTag = FGameplayTag::RequestGameplayTag(FName("State.Gathering"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(GatheringTag, EGameplayTagEventType::AnyCountChange)
			.AddUObject(this, &UPP_GatherAbility::OnGatheringTagRemoved);
	}

	// 启动采集计时器
	if (GatherTarget)
	{
		if (UWorld* World = GetWorld())
		{
			World->GetTimerManager().SetTimer(
				GatherTimerHandle,
				this,
				&UPP_GatherAbility::OnGatherTimerCompleted,
				GatherTarget->GetGatherTime(),
				false
			);
		}
	}
}

void UPP_GatherAbility::OnGatherTimerCompleted()
{
	UE_LOG(LogTemp, Log, TEXT("[GatherAbility] OnGatherTimerCompleted: target=%s"),
		GatherTarget ? *GatherTarget->GetName() : TEXT("null"));

	// 触发采集物的完成逻辑
	if (GatherTarget && CachedActorInfo)
	{
		AActor* AvatarActor = CachedActorInfo->AvatarActor.Get();
		GatherTarget->CompleteGather(AvatarActor);
	}

	// 正常结束能力（非取消）
	if (CachedActorInfo)
	{
		EndAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true, false);
	}
}

void UPP_GatherAbility::OnGatheringTagRemoved(const FGameplayTag Tag, int32 NewCount)
{
	// Tag数量降为0表示被外部移除（受伤打断等）
	if (NewCount == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("[GatherAbility] State.Gathering tag removed, cancelling gather"));

		if (CachedActorInfo)
		{
			CancelAbility(CachedHandle, CachedActorInfo, CachedActivationInfo, true);
		}
	}
}

void UPP_GatherAbility::EndAbility(const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayAbilityActivationInfo ActivationInfo,
	bool bReplicateEndAbility, bool bWasCancelled)
{
	// 清理计时器
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(GatherTimerHandle);
	}

	// 如果被取消，通知采集物取消
	if (bWasCancelled && GatherTarget && ActorInfo)
	{
		AActor* AvatarActor = ActorInfo->AvatarActor.Get();
		GatherTarget->Server_CancelGather(AvatarActor);

		UE_LOG(LogTemp, Log, TEXT("[GatherAbility] EndAbility cancelled: target=%s"),
			GatherTarget ? *GatherTarget->GetName() : TEXT("null"));
	}

	// 停止蒙太奇
	if (GatherMontage && ActorInfo)
	{
		if (ACharacter* Character = Cast<ACharacter>(ActorInfo->AvatarActor.Get()))
		{
			if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
			{
				AnimInstance->Montage_Stop(0.2f, GatherMontage);
			}
		}
	}

	// 取消 Tag 变化监听
	if (ActorInfo && ActorInfo->AbilitySystemComponent.IsValid())
	{
		FGameplayTag GatheringTag = FGameplayTag::RequestGameplayTag(FName("State.Gathering"));
		ActorInfo->AbilitySystemComponent->RegisterGameplayTagEvent(GatheringTag, EGameplayTagEventType::AnyCountChange)
			.RemoveAll(this);
	}

	// 清除采集目标引用
	GatherTarget = nullptr;
	CachedActorInfo = nullptr;

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
