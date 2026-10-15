// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
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
