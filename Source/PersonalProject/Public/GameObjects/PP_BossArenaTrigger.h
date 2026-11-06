
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PP_BossArenaTrigger.generated.h"

class UBoxComponent;
class APP_BossCharacter;
class APP_BossArenaGate;

/**
 * Boss 场地触发器。
 * 玩家进入 Box 时 → 给 Boss AddTag(BossArenaArmed) + 关 ArenaGate 碰撞，
 * Boss 据此激活 SearchForTarget GA，场地门落下锁住区域。
 * 关卡中通过 Blueprint 子类放置，需配置 BossRef 和 GateRef。
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API APP_BossArenaTrigger : public AActor
{
	GENERATED_BODY()

public:
	APP_BossArenaTrigger();

protected:
	virtual void BeginPlay() override;

	/** 触发盒：玩家进入即激活场地 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|BossArena")
	TObjectPtr<UBoxComponent> BoxComponent;

	/** 关联的 Boss（蓝图配置，也可在 BeginPlay 自动查找） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|BossArena")
	TObjectPtr<APP_BossCharacter> BossRef;

	/** 关联的场地门（蓝图配置，也可在 BeginPlay 自动查找） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|BossArena")
	TObjectPtr<APP_BossArenaGate> GateRef;

	/** 已经触发过（避免重复触发） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|BossArena")
	bool bArenaArmed{false};

	/** 场地激活后触发（蓝图覆盖，比如播放过场） */
	UFUNCTION(BlueprintImplementableEvent, Category = "PP|BossArena")
	void OnArenaEntered();

	/** OverlapBegin 回调 */
	UFUNCTION()
	void OnOverlapBegin(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);
};
