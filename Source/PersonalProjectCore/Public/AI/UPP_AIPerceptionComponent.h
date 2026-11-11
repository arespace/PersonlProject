
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "UPP_AIPerceptionComponent.generated.h"

class AAIController;
class APP_BaseCharacter;

/**
 * 敌人 AI 感知组件：全向搜敌（不限朝向，仅要求视线未被遮挡）、脱战 leash、仇恨粘性。
 * 挂在 EnemyCharacter 上：PP_EnemyCharacter::BeginPlay 会自动补建（蓝图未挂时）并 StartSearching()。
 * 对外暴露 CurrentTarget 供 SearchForTarget GA 直接读取。
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECTCORE_API UPP_AIPerceptionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_AIPerceptionComponent();

	/** 启动/重启搜索定时器（幂等；运行时补建组件后需显式调用） */
	UFUNCTION(BlueprintCallable, Category = "PP|AI|Perception")
	void StartSearching();

	/** 用敌人数据行覆盖感知参数并重启搜索定时器；不调用则保留组件/蓝图上的原值 */
	void ApplySearchConfig(float InSearchRadius, float InLeashRange, float InMaxHeightDiff,
		float InTargetLostGraceTime, float InSearchInterval);

	/** 搜索半径（cm）：超过这个距离不考虑 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AI|Perception")
	float SearchRadius{1500.f};

	/** 脱战 leash 距离：目标超过这个距离强制清空 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AI|Perception")
	float LeashRange{3500.f};

	/** 高度差容忍：目标比感知者高/低超过此值忽略 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AI|Perception")
	float MaxHeightDiff{500.f};

	/** 仇恨粘性窗口：目标丢失后这段时间内仍视为有效（避免视野边缘抖动换目标） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AI|Perception")
	float TargetLostGraceTime{0.5f};

	/** 搜索间隔（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|AI|Perception")
	float SearchInterval{0.15f};

	/** 当前锁定目标 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|AI|Perception")
	TWeakObjectPtr<APP_BaseCharacter> CurrentTarget;

	/** 是否有有效目标 */
	UFUNCTION(BlueprintCallable, Category = "PP|AI|Perception")
	bool HasTarget() const { return CurrentTarget.IsValid(); }

	/** 是否处于脱战 leash 状态 */
	UFUNCTION(BlueprintCallable, Category = "PP|AI|Perception")
	bool IsLeashed() const { return bLeashed; }

	/** 是否可以攻击（有目标 + 距离在 Acceptance 内） */
	UFUNCTION(BlueprintCallable, Category = "PP|AI|Perception")
	bool CanAttackNow(float AcceptanceDistance) const;

	/** 强制清空当前目标（EnemyReset 等场景） */
	UFUNCTION(BlueprintCallable, Category = "PP|AI|Perception")
	void ClearTarget();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 搜索一次目标（由 Timer 定期调用） */
	UFUNCTION()
	void Search();

private:
	/** 脱战状态标记 */
	bool bLeashed{false};

	/** 上一次目标有效时间（粘性窗口用） */
	float LastTargetValidTime{-1.f};

	/** 搜索 Timer */
	FTimerHandle SearchTimerHandle;

	/** 拿所属 AIController */
	AAIController* GetOwningAIController() const;

	/** 感知者 Location */
	FVector GetPerceiverLocation() const;
};
