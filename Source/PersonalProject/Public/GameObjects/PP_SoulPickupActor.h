
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PP_SoulPickupActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPointLightComponent;
class APP_PlayerState;

/**
 * 死亡掉落的魂。
 * 玩家死亡时由 APP_PlayerCharacter 在死亡位置生成，绑定该玩家的 PlayerState，
 * 只有拥有者走到范围内才会自动拾回金币。再次死亡时旧魂由玩家角色直接销毁（相当于丢失）。
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API APP_SoulPickupActor : public AActor
{
	GENERATED_BODY()

public:
	APP_SoulPickupActor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	/** 服务端初始化：绑定拥有者并设置掉落金额（SpawnActor 后立即调用） */
	void InitializeSoul(APP_PlayerState* InOwnerState, int32 InAmount);

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Soul")
	int32 GetAmount() const { return Amount; }

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnPickupRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 拾取范围（同时也是根组件，负责重叠检测） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Soul")
	TObjectPtr<USphereComponent> PickupRange;

	/** 占位可视球体（可在蓝图里换成正式特效/网格） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Soul")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/** 发光点光源，便于在场景里找到魂 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Soul")
	TObjectPtr<UPointLightComponent> Glow;

	/** 掉落的金币数量 */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "PP|Soul")
	int32 Amount{0};

	/** 拥有者（只有该玩家的角色能拾回） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "PP|Soul")
	TObjectPtr<APP_PlayerState> OwnerState;
};
