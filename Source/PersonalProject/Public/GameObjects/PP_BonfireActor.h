// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PP_InteractableInterface.h"
#include "PP_BonfireActor.generated.h"

class UStaticMeshComponent;
class UPointLightComponent;
class USceneComponent;
class UBoxComponent;

/**
 * 篝火（存档点）。
 * 玩家按交互键（E）在篝火休息：
 *   1. 把重生点记录到该玩家的 PlayerState；
 *   2. 回满生命/法力/耐力（走 ResetAttributes）；
 *   3. 死亡后由 ServerRespawn 传送到该重生点。
 * 关卡中通过 Blueprint 子类放置，可替换 Mesh、调节光圈与重生点位置。
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API APP_BonfireActor : public AActor, public IPP_Interactable
{
	GENERATED_BODY()

public:
	APP_BonfireActor();

	//~ Begin IPP_Interactable Interface
	virtual void Interact(APlayerController* Interactor) override;
	virtual bool CanInteract(APlayerController* Interactor) const override;
	//~ End IPP_Interactable Interface

	/** 服务器执行：在该篝火休息（记录重生点 + 回满属性）。
	 *  注意：篝火是关卡 Actor，没有 owning connection，不能直接声明 Server RPC，
	 *  必须由拥有者的 PlayerController 转发 Server RPC 进来。 */
	void Rest(APlayerController* Interactor);

protected:
	/** 篝火占位网格（可在蓝图里替换） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Bonfire")
	TObjectPtr<UStaticMeshComponent> Mesh;

	/**
	 * 交互碰撞盒（不可见）。
	 * 独立于可视 Mesh 提供 Visibility 命中，这样即使蓝图中把 Mesh 换成没有碰撞的模型，
	 * 按 E 的相机射线依然能命中篝火。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Bonfire")
	TObjectPtr<UBoxComponent> InteractionCollision;

	/** 篝火光照 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Bonfire")
	TObjectPtr<UPointLightComponent> Light;

	/** 重生点：玩家复活时传送到的位置（可在蓝图里调整相对偏移） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Bonfire")
	TObjectPtr<USceneComponent> RespawnPoint;

	/** 允许交互的最大距离（cm） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "PP|Bonfire", meta = (ClampMin = "0.0"))
	float InteractDistance{300.f};
};
