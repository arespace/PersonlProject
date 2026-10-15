// 采集系统 — 可交互采集物Actor
// 世界中可被玩家长按采集的物品节点，支持多次采集、耗尽重生、多人互斥

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interaction/PP_InteractableInterface.h"
#include "PP_GatherableActor.generated.h"

class USphereComponent;
class UStaticMeshComponent;
class UPP_InventoryComponent;

/**
 * 采集完成全局委托
 * 任务系统将来通过此委托监听采集事件
 * @param Gatherer 执行采集的玩家Actor
 * @param ItemID   产出物品ID
 * @param Count    产出数量
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnGatherCompleted, AActor*, Gatherer, FName, ItemID, int32, Count);

/**
 * 可交互采集物Actor
 *
 * 职责：
 *   1. 提供世界中可视化的采集物（静态网格 + 交互范围球）
 *   2. 当玩家进入交互范围时通知UI显示提示
 *   3. 接收Server RPC执行采集/取消逻辑
 *   4. 支持多次采集，耗尽后隐藏并在指定时间后重生
 *   5. 防止多人同时采集同一节点
 *   6. 采集完成后通过全局委托广播事件（供任务系统接入）
 *
 * 使用方式：在关卡中放置此Actor，配置 ItemID/数量/时间等参数
 */
UCLASS(Blueprintable)
class PERSONALPROJECT_API APP_GatherableActor : public AActor, public IPP_Interactable
{
	GENERATED_BODY()

public:
	APP_GatherableActor();

	//~ Begin IPP_Interactable Interface
	virtual void Interact(APlayerController* Interactor) override;
	virtual bool CanInteract(APlayerController* Interactor) const override;
	//~ End IPP_Interactable Interface

	//~ Begin AActor Interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End AActor Interface

	// ===== 配置属性 =====

	/** 采集产出的物品ID（对应 DataTable 中的行名） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config")
	FName ItemID;

	/** 每次采集产出数量 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config", meta = (ClampMin = "1"))
	int32 GatherCount = 1;

	/** 单次采集耗时（秒），即进度条总长度 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config", meta = (ClampMin = "0.1"))
	float GatherTime = 2.0f;

	/** 可被采集的最大次数，耗尽后进入重生等待 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config", meta = (ClampMin = "1"))
	int32 MaxGatherTimes = 3;

	/** 耗尽后重生等待时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Gather|Config", meta = (ClampMin = "1.0"))
	float RespawnTime = 30.0f;

	// ===== 组件 =====

	/** 可视化网格组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gather|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** 交互范围检测球（玩家进入/离开时触发UI提示） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Gather|Components")
	TObjectPtr<USphereComponent> InteractionRange;

	// ===== 网络状态 =====

	/** 当前正在采集此节点的玩家（nullptr表示空闲可采集） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Gather|State")
	TObjectPtr<AActor> CurrentGatherer;

	// ===== Server RPC =====

	/**
	 * 客户端请求开始采集
	 * 服务器验证：节点是否空闲、剩余采集次数是否足够
	 * @param Gatherer 发起采集的玩家Pawn
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Gather")
	void Server_StartGather(AActor* Gatherer);

	/**
	 * 客户端请求取消采集（松开按键/受伤/移动）
	 * @param Gatherer 取消采集的玩家Pawn
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Gather")
	void Server_CancelGather(AActor* Gatherer);

	/**
	 * 蓝图调用：通知任务系统已拾取此采集物
	 * 用于 Overlap 自动拾取场景（蓝图 ReceiveActorBeginOverlap 中，Apply GE 之后、Destroy 之前调用）
	 * @param Gatherer 拾取此采集物的玩家 Actor
	 */
	UFUNCTION(BlueprintCallable, Category = "Gather")
	void NotifyPickup(AActor* Gatherer);

	/**
	 * 采集完成（由GatherAbility计时结束后调用）
	 * 服务器执行：扣减剩余次数、添加物品到库存、广播委托
	 * @param Gatherer 完成采集的玩家Pawn
	 */
	void CompleteGather(AActor* Gatherer);

	// ===== 状态查询 =====

	/** 该采集节点当前是否可被采集（空闲且未耗尽） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gather")
	bool IsAvailable() const;

	/** 获取采集所需时间 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Gather")
	float GetGatherTime() const { return GatherTime; }

	// ===== 全局委托 =====

	/** 采集完成全局委托（供任务系统等外部系统监听） */
	static FOnGatherCompleted OnGatherCompletedDelegate;

protected:
	/** 进入交互范围回调 */
	UFUNCTION()
	void OnInteractionRangeBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	/** 离开交互范围回调 */
	UFUNCTION()
	void OnInteractionRangeEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex);

private:
	/** 剩余可采集次数 */
	UPROPERTY(Replicated)
	int32 RemainingGatherTimes;

	/** 重生计时器句柄 */
	FTimerHandle RespawnTimerHandle;

	/** 重生逻辑：恢复网格可见与碰撞 */
	void Respawn();

	/** 耗尽逻辑：隐藏网格并启动重生计时器 */
	void Deplete();
};
