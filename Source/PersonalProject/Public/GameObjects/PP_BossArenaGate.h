
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PP_BossArenaGate.generated.h"

class UStaticMeshComponent;
class UBoxComponent;
class APP_BossCharacter;

/**
 * Boss 场地门。
 * 初始状态：禁用碰撞（门开）。BossArenaTrigger 触发时 SetActorEnableCollision(true) 关门。
 * 监听 EventBus 的 FPP_BossDeadEvent → 解锁碰撞（开门）+ 蓝图事件播放开门动画。
 * 只响应已关联 Boss（LinkedBoss，由场地触发器在 BeginPlay 绑定）的死亡事件。
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API APP_BossArenaGate : public AActor
{
	GENERATED_BODY()

public:
	APP_BossArenaGate();

	/** 关联本场地的 Boss；绑定后只响应该 Boss 的死亡事件 */
	void SetLinkedBoss(APP_BossCharacter* InBoss) { LinkedBoss = InBoss; }

	/** 当前关联的 Boss（未绑定时为 nullptr） */
	APP_BossCharacter* GetLinkedBoss() const { return LinkedBoss; }

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

	/** 门的可视网格 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|BossGate")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 门的阻挡碰撞（独立于 Mesh，便于蓝图里替换无碰撞模型） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|BossGate")
	TObjectPtr<UBoxComponent> CollisionComponent;

	/** 初始是否关闭（默认 false，关卡蓝图里也可以覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|BossGate")
	bool bInitiallyClosed{false};

	/** 本场地门关联的 Boss（通常由 BossArenaTrigger 自动绑定，也可在蓝图显式指定） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|BossGate")
	TObjectPtr<APP_BossCharacter> LinkedBoss;

	/** Boss 死亡触发，蓝图覆盖播放开门动画 */
	UFUNCTION(BlueprintImplementableEvent, Category = "PP|BossGate")
	void OnBossKilled();

private:
	/** EventBus 订阅句柄 */
	FDelegateHandle BossDeadHandle;

	/** EventBus 回调：Boss 死亡 → 解锁 + 动画 */
	void HandleBossDead(const struct FPP_BossDeadEvent& Event);
};
