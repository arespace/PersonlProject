// 库存系统 — 库存动作接口
// UI 通过本接口发起穿戴/脱下/使用请求，具体实现转发到组件的 Server RPC。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_ItemTypes.h"
#include "PP_InventoryActions.generated.h"

UINTERFACE(BlueprintType)
class UPP_InventoryActions : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_InventoryActions — 库存写操作能力接口
 * 实现方：UPP_InventoryComponent
 * 调用方：UI 模块（物品格子点击穿戴/使用、装备槽点击脱下）
 *
 * 约定：这些方法均为「请求」，实现方内部转发到 Server RPC，由服务器权威执行。
 */
class PERSONALPROJECTINVENTORY_API IPP_InventoryActions
{
	GENERATED_BODY()

public:
	/** 请求穿戴指定物品 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Actions")
	void RequestEquipItem(FName ItemID);

	/** 请求脱下指定槽位的装备 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Actions")
	void RequestUnequipItem(EPP_EquipSlot Slot);

	/** 请求使用指定消耗品 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Actions")
	void RequestUseConsumable(FName ItemID);
};
