// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AlsCharacter.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

#include "PP_BaseCharacter.generated.h"

namespace PersonalTags
{
	extern PERSONALPROJECTCORE_API const FName Player;
}

class UAttributeSet;
class UGameplayAbility;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class PERSONALPROJECTCORE_API APP_BaseCharacter : public AAlsCharacter, public IAbilitySystemInterface 
{
	GENERATED_BODY()

public:
	APP_BaseCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const {return nullptr;};
	bool IsAlive() const {return bAlive;}
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }

	/**
	 * 是否可以被伤害：检查 Invulnerable（翻滚 i-frames）、Dead（已死亡）、
	 * BossPhaseTransition（Boss 阶段切换无敌）三个标签，任一存在则返回 false。
	 * 不受 AActor::bCanBeDamaged 成员影响——那些标签是 GAS 运行时状态。
	 */
	virtual bool CanBeDamaged() const;
	
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Death")
	virtual void HandleRespawn();

	/** 死亡 GA 在服务端调用：统一走 ServerRespawn 复活流程（不再由 GA 直接传送/重置） */
	void RespawnCharacter();
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Attributes")
	void ResetAttributes();
	
	UFUNCTION(BlueprintImplementableEvent)
	void RotateToTarget(AActor* RotateTarget);
	
	UPROPERTY(EditAnywhere, Category = "Crash|AI")
	float SearchRange{1000.f};

	// 是否强制套用 ALS 动画蓝图 AB_Als。
	// ALS Tick 有一条守卫：没有 UAlsAnimationInstance 就整体提前 return（含 RefreshView/相机刷新），
	// 因此需要 ALS 相机与视角的角色（玩家）必须为 true；
	// 使用自带动画蓝图且不需要 ALS 相机的角色（敌人）保持 false，避免骨架不匹配导致动作别扭。
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Als")
	bool bForceAlsAnimationBlueprint{false};
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;
	// ALS 的模拟代理会忽略服务端复制的旋转（ALS 自行控制角色朝向）。
	// 非 ALS 角色（敌人）没有这套机制，必须恢复默认行为，否则客户端上敌人永远不转向。
	virtual void PostNetReceiveLocationAndRotation() override;

	void GiveStartupAbilities();
	void InitializeAttributes() const;
	
	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	virtual void HandleDeath();

	// 复活由服务端在 HandleDeath 里启动计时器触发，不再依赖死亡 GA 的蒙太奇回调——
	// 玩家死亡会 StartRagdolling，网格进入物理模拟后蒙太奇任务不会播放，
	// 其回调（OnCompleted/Interrupted/Cancelled）都不广播，服务端 GA 实例会卡死。
	// ShouldRespawn：玩家恒 true；敌人为 false（敌人不 Ragdoll，蒙太奇链路正常，复活在死亡 GA 里处理）。
	virtual bool ShouldRespawn() const { return false; }
	virtual void ServerRespawn();

	/**
	 * 取复活传送变换。
	 * 基类默认查找 PlayerStart；玩家角色覆盖此函数以优先使用篝火记录的 PlayerState 重生点。
	 */
	virtual bool GetRespawnTransform(FTransform& OutTransform);

	// 复活后通知 owning 客户端恢复输入，并在客户端本地同步落点：
	// 只靠服务端复制位移时，客户端可能还停留在死亡点，恢复输入后先按旧位置预测再被拉回，
	// 表现为复活后移动卡顿/回弹。这里把服务端最终落点直接传给客户端做一次本地 Teleport。
	UFUNCTION(Client, Reliable)
	void ClientOnRespawn(FVector RespawnLocation, float RespawnYaw);

	/** ServerRespawn 计算出的落点，供 HandleRespawn 发给客户端（无篝火点时为空） */
	FVector PendingRespawnLocation{FVector::ZeroVector};
	float PendingRespawnYaw{0.f};
	bool bHasPendingRespawnLocation{false};

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Death")
	float RespawnDelay{3.f};

	FTimerHandle RespawnTimerHandle;

	// 受击反应：临时切换到 ALS Injured 覆盖层，短暂后恢复 Default。
	// 死亡（血量归零）走 HandleDeath→StartRagdolling，不触发受击反应。
	virtual void PlayHitReaction();
	void ResetHitReactionOverlay();
	FTimerHandle HitReactionTimerHandle;

	// 复活后移动诊断采样（仅客户端本地控制角色）：
	// 2 秒内每 0.1 秒输出一次位置/速度/输入/移动模式，用来定位复活后的卡顿来自
	// 本地预测、服务端校正还是残留的移动输入。
	float RespawnMovementDebugEndTime{-1.f};
	float RespawnMovementDebugNextLogTime{0.f};
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Replicated)
	bool bAlive = true;
	
};
