// 锁定系统组件 — 目标选择、相机朝向、解除锁定

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "PP_LockOnComponent.generated.h"

class AActor;
class APP_PlayerCharacter;

/**
 * 玩家锁定组件。
 * 仅本地玩家生效：目标选择 / 相机朝向 / 角色朝向；服务端依靠 ALS 视角同步。
 * Tick 由角色在 ALS Tick 之前手动驱动，保持原有更新顺序。
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_LockOnComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_LockOnComponent();

	/** 切换锁定：已锁定则解锁，否则搜索并锁定最近的敌人 */
	UFUNCTION(BlueprintCallable, Category = "PP|LockOn")
	void ToggleLockOn();

	/** 解除锁定并恢复锁定前的旋转模式 */
	UFUNCTION(BlueprintCallable, Category = "PP|LockOn")
	void ReleaseLockOn();

	/** 当前锁定的敌人（无锁定时为 nullptr） */
	UFUNCTION(BlueprintPure, Category = "PP|LockOn")
	AActor* GetLockOnTarget() const { return LockOnTarget.Get(); }

	/** 锁定目标的世界坐标焦点（胸口高度，供 HUD 绘制标记） */
	UFUNCTION(BlueprintPure, Category = "PP|LockOn")
	FVector GetLockOnFocusLocation() const;

	UFUNCTION(BlueprintPure, Category = "PP|LockOn")
	bool IsLockedOn() const { return LockOnTarget.IsValid(); }

	/** 由角色 Tick 在 ALS 之前调用，本地玩家每帧驱动相机朝向目标并校验目标有效性 */
	void UpdateLockOn(float DeltaTime);

	/**
	 * 迁移用：用角色上旧属性里的非默认值覆盖组件参数。
	 * @return 是否有任意一项被覆盖（有则说明蓝图里存在旧配置）
	 */
	bool ApplyLegacySettings(float InSearchRadius, float InBreakDistance, float InMaxScreenAngle,
	                         float InCameraInterpolationSpeed, float InFocusHeight);

	/** 搜索敌人的最大半径（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|LockOn", Meta = (ClampMin = 0))
	float LockOnSearchRadius{1200.f};

	/** 超过该距离自动解除锁定（cm，需大于搜索半径形成滞回） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|LockOn", Meta = (ClampMin = 0))
	float LockOnBreakDistance{1600.f};

	/** 目标必须位于相机前方这个半角内才会被锁定（度） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|LockOn", Meta = (ClampMin = 0, ClampMax = 180))
	float LockOnMaxScreenAngle{60.f};

	/** 相机朝向目标的插值速度（越大转得越快） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|LockOn", Meta = (ClampMin = 0))
	float LockOnCameraInterpolationSpeed{10.f};

	/** 目标焦点相对敌人 Actor 原点的高度偏移（cm，取胸口高度） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|LockOn")
	float LockOnFocusHeight{60.f};

private:
	APP_PlayerCharacter* GetOwnerCharacter() const;

	/** 在搜索半径/视锥内寻找最近的可见敌人 */
	AActor* FindLockOnTarget();

	/** 锁定目标是否仍然有效（存活且未超出解除距离） */
	bool IsLockOnTargetValid(const AActor* Target) const;

	/** 当前锁定目标（仅本地玩家维护，不参与网络复制） */
	TWeakObjectPtr<AActor> LockOnTarget;

	/** 锁定前的 ALS 期望旋转模式，解除锁定时恢复 */
	FGameplayTag PreLockOnRotationMode;
};
