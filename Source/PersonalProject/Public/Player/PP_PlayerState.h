
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerState.h"

#include "PP_PlayerState.generated.h"


class UAttributeSet;
class UAbilitySystemComponent;
class UPP_InventoryComponent;
class UPP_QuestComponent;

UCLASS()
class PERSONALPROJECT_API APP_PlayerState : public APlayerState, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APP_PlayerState();
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;	
	UAttributeSet* GetAttributeSet() const{ return AttributeSet;};

	/** 获取选中的角色ID（从大厅迁移） */
	FName GetSelectedCharacterId() const { return SelectedCharacterId; }

	/** 设置选中的角色ID */
	void SetSelectedCharacterId(FName InCharacterId) { SelectedCharacterId = InCharacterId; }

	/** 获取库存与装备组件（服务端权威，所有客户端可读） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	UPP_InventoryComponent* GetInventoryComponent() const { return InventoryComponent; }

	/** 获取任务管理组件 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Quest")
	UPP_QuestComponent* GetQuestComponent() const { return QuestComponent; }

	// ===== 篝火重生点 =====

	/** 设置重生点（服务器在篝火休息时调用） */
	void SetCheckpoint(const FTransform& InTransform);

	/** 是否已经记录过重生点 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Checkpoint")
	bool HasCheckpoint() const { return bHasCheckpoint; }

	/** 获取重生点（未记录时返回 Identity） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Checkpoint")
	FTransform GetCheckpointTransform() const { return CheckpointTransform; }

	// ===== 计分板统计（服务端权威，随 PlayerState 复制到所有客户端） =====

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Stats")
	int32 GetKills() const { return Kills; }

	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "PP|Stats")
	int32 GetDeaths() const { return Deaths; }

	/** 记一次击杀：击杀数 +1，并累加到父类 APlayerState 的 Score（当前口径为每次 1 分） */
	void AddKill(int32 ScorePoints = 1);

	/** 记一次死亡：死亡数 +1 */
	void AddDeath();

protected:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	
	UPROPERTY(VisibleAnywhere, Category = "PP|Abilities")
	TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;
	
	UPROPERTY()
	TObjectPtr<UAttributeSet> AttributeSet;

	/** 库存与装备组件，挂载在 PlayerState 上以保留跨地图数据 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_InventoryComponent> InventoryComponent;

	/** 任务管理组件，挂载在 PlayerState 上 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Quest", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_QuestComponent> QuestComponent;

	// ===== SeamlessTravel数据桥接 =====

	/** 从大厅携带到战斗地图的角色选择ID */
	UPROPERTY(Replicated)
	FName SelectedCharacterId;

	/** 是否已经记录过篝火重生点 */
	UPROPERTY(Replicated)
	bool bHasCheckpoint{false};

	/** 篝火重生点（玩家死亡复活时传送到的位置） */
	UPROPERTY(Replicated)
	FTransform CheckpointTransform{FTransform::Identity};

	/** 击杀数（计分板统计） */
	UPROPERTY(Replicated)
	int32 Kills{0};

	/** 死亡数（计分板统计） */
	UPROPERTY(Replicated)
	int32 Deaths{0};

	// 得分直接使用父类 APlayerState::Score（float，已由父类复制），此处不再重复定义
};
