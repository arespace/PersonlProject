// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Characters/PP_EnemyCharacter.h"
#include "PP_BossCharacter.generated.h"

class UPP_AttributeSet;
struct FOnAttributeChangeData;

/**
 * Boss 基类。继承 APP_EnemyCharacter（复用 GAS/攻击 GA/属性/死亡链路），扩展：
 *   - Boss 标记：BeginPlay 自动 AddTag(Pettags.Status.Boss)，HUD 据此识别并画血条
 *   - 阶段切换：按血量百分比阈值自动切换阶段，切换期间短暂加 BossPhaseTransition 无敌
 *   - 血量广播：BossHealthPercent / BossName / CurrentPhase 作为 replicated 属性，HUD 直接读
 *   - 奖励：死亡时击杀金币按 BossGoldMultiplier 倍率发放
 * 蓝图侧只需要配模型、GA、阶段阈值和奖励倍率。
 */
UCLASS()
class PERSONALPROJECT_API APP_BossCharacter : public APP_EnemyCharacter
{
	GENERATED_BODY()

public:
	APP_BossCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Boss")
	const FName& GetBossName() const { return BossName; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Boss")
	int32 GetCurrentBossPhase() const { return CurrentPhase; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Boss")
	float GetBossHealthPercent() const { return BossHealthPercent; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Boss")
	float GetBossGoldMultiplier() const { return BossGoldMultiplier; }

protected:
	virtual void BeginPlay() override;
	virtual void HandleDeath() override;

	/** 监听 Health 属性变化，服务器驱动阶段切换与血量广播（C++ AddUObject 绑定，非反射） */
	void OnBossHealthChanged(const FOnAttributeChangeData& Data);

	/** 检查并应用阶段切换（服务器内调用） */
	void ApplyPhaseTransitionIfNeeded(float NewHealthPercent);

	/** 阶段切换后触发（蓝图覆盖以播放动画/切换 GA 配置） */
	UFUNCTION(BlueprintImplementableEvent, Category = "PP|Boss")
	void OnBossPhaseChanged(int32 NewPhase, int32 OldPhase);

	/** Boss 显示名（出现在顶部血条） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Replicated, Category = "PP|Boss")
	FName BossName{TEXT("Ancient Boss")};

	/** 血量降到哪些百分比时切阶段（升序，值在 0~1 之间）。默认 [0.75, 0.50, 0.25] 共四段 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Boss", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	TArray<float> BossPhaseThresholds;

	/** 当前阶段（1 起） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "PP|Boss")
	int32 CurrentPhase{1};

	/** 当前血量百分比 0~1，HUD 直接读这个值来画血条 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Replicated, Category = "PP|Boss")
	float BossHealthPercent{1.f};

	/** 阶段切换无敌时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Boss", meta = (ClampMin = "0.0"))
	float PhaseTransitionInvulnerableTime{1.5f};

	/** 死亡时击杀金币倍率（相对敌人默认 1 倍） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Boss", meta = (ClampMin = "1.0"))
	float BossGoldMultiplier{10.f};

private:
	UFUNCTION()
	void OnPhaseTransitionInvulnerableEnd();

	/** 当前阶段切换剩余时间句柄，用于阶段切换时重置计时器 */
	FTimerHandle PhaseInvulnerableTimerHandle;

	/** 缓存 MaxHealth，避免每次算 Percent 都取 AttributeSet */
	float CachedMaxHealth{1.f};
};
