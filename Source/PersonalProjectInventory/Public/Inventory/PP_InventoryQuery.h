// 库存系统 — 库存只读查询接口
// 解耦 UI 对 UPP_InventoryComponent 具体类的直接持有：
// UI 通过本接口读取库存数据，通过 EventBus 的 FPP_InventoryUpdatedEvent 等事件感知变更。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_ItemData.h"
#include "Data/PP_ItemTypes.h"
#include "PP_InventoryQuery.generated.h"

UINTERFACE(BlueprintType)
class UPP_InventoryQuery : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_InventoryQuery — 库存数据只读查询能力接口
 * 实现方：UPP_InventoryComponent（挂载在 APP_PlayerState 上）
 * 调用方：UI 模块（背包、装备栏、提示框、金币显示等）
 *
 * 约定：本接口只读，不提供任何写操作；写操作走 IPP_InventoryActions。
 * 物品定义（FPP_ItemInfo）属全局数据，走 DataManager 查询，不在此接口内。
 */
class PERSONALPROJECTINVENTORY_API IPP_InventoryQuery
{
	GENERATED_BODY()

public:
	/** 查询背包当前所有物品实例 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Query")
	TArray<FPP_ItemInstance> QueryItems();

	/** 查询所有已装备物品 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Query")
	TArray<FPP_EquippedItem> QueryEquippedItems();

	/** 查询当前金币数量 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Query")
	int32 QueryGold();

	/** 查询库存中某物品的总数 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Query")
	int32 QueryItemCount(FName ItemID);

	/** 查询指定槽位当前已装备的物品（无则返回空 FPP_EquippedItem） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Inventory|Query")
	FPP_EquippedItem QueryEquippedItemInSlot(EPP_EquipSlot Slot);
};
