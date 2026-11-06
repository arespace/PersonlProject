
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_ItemTypes.h"
#include "PP_InteractableInterface.generated.h"

class APlayerController;

/**
 * 可交互对象接口
 * 所有可被玩家交互的 Actor（NPC、采集物、宝箱、传送门等）实现此接口。
 * PlayerController::Interact 只需 Cast 此接口，无需关心具体类型。
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UPP_Interactable : public UInterface
{
	GENERATED_BODY()
};

class IPP_Interactable
{
	GENERATED_BODY()

public:
	/** 执行交互，由 PlayerController 调用 */
	virtual void Interact(APlayerController* Interactor) = 0;

	/** 是否可以交互（默认 true，子类可覆盖做条件判断） */
	virtual bool CanInteract(APlayerController* Interactor) const { return true; }
};

/**
 * 可重置接口：篝火休息/区域重置时，敌人、采集物等 Actor 需要回到初始状态。
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UPP_Resettable : public UInterface
{
	GENERATED_BODY()
};

class PERSONALPROJECTCORE_API IPP_Resettable : public IInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Respawn")
	void ResetToSpawn();
	virtual void ResetToSpawn_Implementation() {}
};

/**
 * 可硬直接口：敌人被累计伤害打崩时进入 stagger 状态，播放受击/倒地蒙太奇。
 */
UINTERFACE(MinimalAPI, Blueprintable)
class UPP_Staggerable : public UInterface
{
	GENERATED_BODY()
};

class PERSONALPROJECTCORE_API IPP_Staggerable : public IInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Combat")
	void PlayStaggered();
	virtual void PlayStaggered_Implementation() {}
};

// ============================================================
// IPP_EquipmentUser — 武器/装备系统的装备变更回调接口
// ============================================================
UINTERFACE(MinimalAPI, Blueprintable)
class UPP_EquipmentUser : public UInterface
{
	GENERATED_BODY()
};

class PERSONALPROJECTCORE_API IPP_EquipmentUser : public IInterface
{
	GENERATED_BODY()

public:
	/**
	 * 装备槽位变更回调
	 * @param SlotType  哪个槽位发生了变化
	 * @param NewItemID 新装备的物品ID（NAME_None 表示卸下）
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Equipment")
	void OnEquipmentChanged(EPP_EquipmentSlotType SlotType, FName NewItemID);
	virtual void OnEquipmentChanged_Implementation(EPP_EquipmentSlotType SlotType, FName NewItemID) {}
};
