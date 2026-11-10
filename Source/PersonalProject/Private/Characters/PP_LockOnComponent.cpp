#include "Characters/PP_LockOnComponent.h"

#include "Camera/PlayerCameraManager.h"
#include "Characters/PP_EnemyCharacter.h"
#include "Characters/PP_PlayerCharacter.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Framework/PP_Log.h"
#include "GameFramework/PlayerController.h"
#include "Utility/AlsGameplayTags.h"

UPP_LockOnComponent::UPP_LockOnComponent()
{
	// 由角色 Tick 在 ALS Tick 之前手动调用 UpdateLockOn，组件自身不挂 Tick
	PrimaryComponentTick.bCanEverTick = false;
}

APP_PlayerCharacter* UPP_LockOnComponent::GetOwnerCharacter() const
{
	return Cast<APP_PlayerCharacter>(GetOwner());
}

void UPP_LockOnComponent::ToggleLockOn()
{
	APP_PlayerCharacter* Character = GetOwnerCharacter();
	if (!Character || !Character->IsLocallyControlled())
	{
		return;
	}

	if (!Character->IsAlive())
	{
		ReleaseLockOn();
		return;
	}

	// 已有锁定则解锁
	if (LockOnTarget.IsValid())
	{
		ReleaseLockOn();
		return;
	}

	if (AActor* Found = FindLockOnTarget())
	{
		LockOnTarget = Found;
		PreLockOnRotationMode = Character->GetDesiredRotationMode();

		// Aiming 旋转模式让角色在静止时也朝向视角（即目标），
		// 移动时则保持下半身朝目标、可横向绕圈。
		Character->SetDesiredRotationMode(AlsRotationModeTags::Aiming);

		UE_LOG(LogPP, Log, TEXT("[LockOn] 锁定目标: %s"), *Found->GetName());
	}
}

FVector UPP_LockOnComponent::GetLockOnFocusLocation() const
{
	const AActor* Target = LockOnTarget.Get();
	return IsValid(Target)
		       ? Target->GetActorLocation() + FVector(0.f, 0.f, LockOnFocusHeight)
		       : FVector::ZeroVector;
}

bool UPP_LockOnComponent::IsLockOnTargetValid(const AActor* Target) const
{
	const APP_PlayerCharacter* Character = GetOwnerCharacter();
	if (!Character || !IsValid(Target))
	{
		return false;
	}

	const APP_BaseCharacter* TargetCharacter = Cast<APP_BaseCharacter>(Target);
	if (!TargetCharacter || !TargetCharacter->IsAlive())
	{
		return false;
	}

	return FVector::DistSquared(Character->GetActorLocation(), Target->GetActorLocation())
		<= FMath::Square(LockOnBreakDistance);
}

AActor* UPP_LockOnComponent::FindLockOnTarget()
{
	APP_PlayerCharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return nullptr;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return nullptr;
	}

	const FVector CharacterLocation = Character->GetActorLocation();
	const FVector CameraLocation = PC->PlayerCameraManager->GetCameraLocation();
	const FVector CameraForward = PC->PlayerCameraManager->GetCameraRotation().Vector();
	const float MinDot = FMath::Cos(FMath::DegreesToRadians(LockOnMaxScreenAngle));

	AActor* BestTarget = nullptr;
	float BestScore = TNumericLimits<float>::Max();

	for (TActorIterator<APP_EnemyCharacter> It(Character->GetWorld()); It; ++It)
	{
		APP_EnemyCharacter* Enemy = *It;
		if (!IsValid(Enemy) || !Enemy->IsAlive() || Enemy->IsHidden())
		{
			continue;
		}

		const float Distance = FVector::Dist(CharacterLocation, Enemy->GetActorLocation());
		if (Distance > LockOnSearchRadius)
		{
			continue;
		}

		const FVector FocusLocation = Enemy->GetActorLocation() + FVector(0.f, 0.f, LockOnFocusHeight);
		const FVector ToTarget = FocusLocation - CameraLocation;
		const float Dot = FVector::DotProduct(CameraForward, ToTarget.GetSafeNormal());
		if (Dot < MinDot)
		{
			continue;
		}

		// 视线校验：忽略自身与目标本身，任何遮挡都视为不可见
		FCollisionQueryParams Params(SCENE_QUERY_STAT(PP_LockOnTrace), false, Character);
		Params.AddIgnoredActor(Character);
		Params.AddIgnoredActor(Enemy);

		FHitResult Hit;
		if (Character->GetWorld()->LineTraceSingleByChannel(Hit, CameraLocation, FocusLocation, ECC_Visibility, Params))
		{
			continue;
		}

		// 综合距离与屏幕角度：越近、越靠近画面中心分越高
		const float AnglePenalty = FMath::Acos(FMath::Clamp(Dot, -1.f, 1.f)) / PI;
		const float Score = Distance / LockOnSearchRadius + AnglePenalty;
		if (Score < BestScore)
		{
			BestScore = Score;
			BestTarget = Enemy;
		}
	}

	return BestTarget;
}

void UPP_LockOnComponent::UpdateLockOn(float DeltaTime)
{
	APP_PlayerCharacter* Character = GetOwnerCharacter();
	if (!Character || !IsLockedOn())
	{
		return;
	}

	if (!Character->IsAlive() || !IsLockOnTargetValid(LockOnTarget.Get()))
	{
		ReleaseLockOn();
		return;
	}

	if (!Character->IsLocallyControlled())
	{
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Character->GetController());
	if (!IsValid(PC) || !IsValid(PC->PlayerCameraManager))
	{
		return;
	}

	// 相机与角色朝向都由 ControlRotation 驱动：ALS 会把本地视角同步到服务端，
	// 服务端因此能据此计算角色朝向，攻击命中判定方向与画面一致。
	const FVector FocusLocation = GetLockOnFocusLocation();
	const FRotator DesiredRotation = (FocusLocation - PC->PlayerCameraManager->GetCameraLocation()).Rotation();
	const FRotator CurrentRotation = PC->GetControlRotation();

	PC->SetControlRotation(FMath::RInterpTo(CurrentRotation, DesiredRotation, DeltaTime, LockOnCameraInterpolationSpeed));
}

void UPP_LockOnComponent::ReleaseLockOn()
{
	APP_PlayerCharacter* Character = GetOwnerCharacter();
	if (!Character)
	{
		return;
	}

	if (!LockOnTarget.IsValid() && !PreLockOnRotationMode.IsValid())
	{
		return;
	}

	const bool bWasLocked = LockOnTarget.IsValid();
	LockOnTarget.Reset();

	if (PreLockOnRotationMode.IsValid())
	{
		if (Character->IsLocallyControlled())
		{
			UE_LOG(LogPP, Log, TEXT("[LockOn] 恢复旋转模式: %s → %s"),
				*Character->GetDesiredRotationMode().ToString(), *PreLockOnRotationMode.ToString());
			Character->SetDesiredRotationMode(PreLockOnRotationMode);
		}
		PreLockOnRotationMode = FGameplayTag();
	}

	if (bWasLocked)
	{
		UE_LOG(LogPP, Log, TEXT("[LockOn] 解除锁定"));
	}
}
