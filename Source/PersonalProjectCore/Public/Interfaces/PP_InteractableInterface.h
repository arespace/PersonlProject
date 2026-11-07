
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_ItemTypes.h"
// 兼容既有引用：装备接口已拆到独立头文件
#include "Interfaces/PP_EquipmentUserInterface.h"
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
