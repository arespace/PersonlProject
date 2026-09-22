// Chen Tianbao uses GAS to create online battle games

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

protected:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:
	
	UPROPERTY(VisibleAnywhere, Category = "Crash|Abilities")
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
};
