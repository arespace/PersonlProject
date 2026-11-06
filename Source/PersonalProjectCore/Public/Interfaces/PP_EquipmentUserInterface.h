// 装备变更回调接口
// 独立头文件：仅实现装备回调的角色不必包含整套交互接口

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_ItemTypes.h"
#include "PP_EquipmentUserInterface.generated.h"

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
