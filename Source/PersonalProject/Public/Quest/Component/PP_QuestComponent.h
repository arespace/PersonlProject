// 任务系统 — 任务管理组件
// 服务器权威的任务接取/放弃/提交/进度追踪，通过网络复制同步至客户端

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Quest/Data/PP_QuestData.h"
#include "PP_QuestComponent.generated.h"

class UDataTable;
class UPP_InventoryComponent;

/** 任务接取事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestAccepted, FName, QuestID);

/** 任务完成事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestCompleted, FName, QuestID);

/** 目标进度更新事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnObjectiveUpdated, FName, QuestID, int32, ObjectiveIndex);

/** 任务失败事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnQuestFailed, FName, QuestID);

/** 任务列表变更事件（客户端 OnRep 时广播，供 UI 刷新） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnQuestsUpdated);

/**
 * 任务管理组件
 *
 * 职责：
 *   1. 维护玩家的活跃任务列表和已完成任务列表
 *   2. 处理任务接取/放弃/提交逻辑（含前置验证）
 *   3. 处理目标进度更新（由事件桥接器调用）
 *   4. 任务完成后发放奖励（调用 InventoryComponent）
 *   5. 支持组队共享进度广播
 *   6. 通过 ReplicatedUsing 在服务器与客户端间同步任务状态
 *
 * 网络模型：所有写操作在服务器执行，客户端通过 OnRep 回调刷新 UI。
 *
 * 使用约定：挂载在 APP_PlayerState 上。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_QuestComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_QuestComponent();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent Interface

	// ===== 任务操作接口（服务器权威） =====

	/**
	 * 接取任务
	 * 验证前置任务是否完成、任务是否已接取/已完成
	 * @param QuestID 要接取的任务ID
	 * @return 是否成功接取
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AcceptQuest(FName QuestID);

	/**
	 * 放弃任务
	 * @param QuestID 要放弃的任务ID
	 * @return 是否成功放弃
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool AbandonQuest(FName QuestID);

	/**
	 * 提交任务
	 * 验证所有目标已完成 → 发放奖励 → 移入已完成列表 → 触发下一主线解锁
	 * @param QuestID 要提交的任务ID
	 * @return 是否成功提交
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	bool SubmitQuest(FName QuestID);

	/**
	 * 更新目标进度
	 * 遍历活跃任务匹配目标类型+TargetID，更新 CurrentCount
	 * @param Type 目标类型
	 * @param TargetID 目标ID
	 * @param Count 增量（默认1）
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void UpdateObjectiveProgress(EPP_QuestObjectiveType Type, FName TargetID, int32 Count = 1);

	/**
	 * 组队共享进度广播
	 * 遍历所有 PlayerState 找队友的 QuestComponent 并更新其 bSharedInTeam 目标
	 */
	UFUNCTION(BlueprintCallable, Category = "Quest")
	void Server_BroadcastTeamProgress(EPP_QuestObjectiveType Type, FName TargetID, int32 Count);

	// ===== 查询接口 =====

	/** 获取当前活跃任务列表 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TArray<FPP_ActiveQuest> GetActiveQuests() const { return ActiveQuests; }

	/** 获取已完成任务ID列表 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	TArray<FName> GetCompletedQuests() const { return CompletedQuestIDs; }

	/** 指定任务是否正在进行 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsQuestActive(FName QuestID) const;

	/** 指定任务是否已完成 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool IsQuestCompleted(FName QuestID) const;

	/** 是否可以接取指定任务（前置满足 + 未激活 + 未完成） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	bool CanAcceptQuest(FName QuestID) const;

	/** 从任务数据表查找任务信息 */
	const FPP_QuestInfo* FindQuestInfo(FName QuestID) const;

	// ===== 委托 =====

	/** 任务接取时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestAccepted OnQuestAccepted;

	/** 任务完成时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestCompleted OnQuestCompleted;

	/** 目标进度更新时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnObjectiveUpdated OnObjectiveUpdated;

	/** 任务失败时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestFailed OnQuestFailed;

	/** 任务列表变更时广播（客户端 OnRep 触发，服务端 Accept/Submit 后也会触发） */
	UPROPERTY(BlueprintAssignable, Category = "Quest|Events")
	FOnQuestsUpdated OnQuestsUpdated;

	// ===== 配置 =====

	/** 任务数据表（FPP_QuestInfo 行结构） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Config")
	TObjectPtr<UDataTable> QuestDataTable;

protected:
	// ===== OnRep 回调 =====

	UFUNCTION()
	void OnRep_ActiveQuests();

	// ===== 复制属性 =====

	/** 活跃任务列表（服务器权威） */
	UPROPERTY(ReplicatedUsing = OnRep_ActiveQuests, BlueprintReadOnly, Category = "Quest")
	TArray<FPP_ActiveQuest> ActiveQuests;

	/** 已完成任务ID列表（服务器权威） */
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Quest")
	TArray<FName> CompletedQuestIDs;

private:
	/** 获取拥有者的库存组件 */
	UPP_InventoryComponent* GetOwningInventoryComponent() const;
};
