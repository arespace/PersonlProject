// 商店系统 — 商店管理组件
// 服务器权威的购买/出售逻辑，验证物品存在性、库存、金币，并广播购买完成事件

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Shop/Data/PP_ShopData.h"
#include "PP_ShopComponent.generated.h"

class UDataTable;
class UPP_InventoryComponent;
class APlayerController;

/** 购买完成事件广播：购买者Actor + 物品ID + 数量 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnPurchaseCompleted, AActor*, Buyer, FName, ItemID, int32, Count);

/** 操作结果广播：通用商店操作结果消息（金币不足、库存不足等） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShopResult, const FText&, Message);

/**
 * 商店管理组件
 *
 * 职责：
 *   1. 维护商品数据表引用与出售折扣配置
 *   2. 处理购买请求：验证商品存在→库存充足→金币充足→扣金币→加物品→广播事件
 *   3. 处理出售请求：验证背包有物品→移除物品→加金币（SellPrice * SellPriceMultiplier）
 *   4. 通过 OnPurchaseCompleted 委托供任务系统监听购买行为
 *
 * 网络模型：Server RPC 执行购买/出售，客户端仅请求。
 *          结果通过 InventoryComponent 的 OnRep 自动同步。
 *
 * 使用约定：挂载在 NPC Actor 或任何需要商店功能的 Actor 上。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_ShopComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_ShopComponent();

	// ===== 购买/出售接口（服务器权威） =====

	/**
	 * 客户端请求购买物品
	 * 服务器执行：验证商品→库存→金币→扣金币→加物品→更新库存→广播事件
	 * @param Buyer 购买者的 PlayerController
	 * @param ItemID 商品物品ID
	 * @param Count 购买数量
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Shop")
	void Server_BuyItem(APlayerController* Buyer, FName ItemID, int32 Count);

	/**
	 * 客户端请求出售物品
	 * 服务器执行：验证背包有物品→移除物品→按折扣计算金币→加金币
	 * @param Seller 出售者的 PlayerController
	 * @param ItemID 出售的物品ID
	 * @param Count 出售数量
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Shop")
	void Server_SellItem(APlayerController* Seller, FName ItemID, int32 Count);

	// ===== 查询接口 =====

	/** 获取当前商店的所有商品列表（供UI使用） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop")
	TArray<FPP_ShopItem> GetShopItems() const;

	/** 获取商品的实际售价（考虑 PriceOverride） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop")
	int32 GetItemBuyPrice(FName ItemID) const;

	/** 获取出售折扣系数 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Shop")
	float GetSellPriceMultiplier() const { return SellPriceMultiplier; }

	// ===== 委托 =====

	/** 购买完成事件广播（任务系统监听：PurchaseItem类型目标追踪） */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Events")
	FOnPurchaseCompleted OnPurchaseCompleted;

	/** 静态购买完成委托：不受 NPC 实例生命周期限制，任务系统通过此委托监听（参照 Gather 系统模式） */
	static FOnPurchaseCompleted OnPurchaseCompletedDelegate;

	/** 操作结果广播（购买/出售失败时通知UI显示提示） */
	UPROPERTY(BlueprintAssignable, Category = "Shop|Events")
	FOnShopResult OnShopResult;

protected:
	// ===== 网络通知 =====

	/** 客户端通知：显示操作结果消息（金币不足、库存不足等） */
	UFUNCTION(Client, Reliable, Category = "Shop")
	void Client_OnShopResult(const FText& Message);

	// ===== 配置 =====

	/** 商品数据表（FPP_ShopItem 行结构），定义商店售卖的物品清单 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Config")
	TObjectPtr<UDataTable> ShopDataTable;

	/** 物品数据表引用（FPP_ItemInfo行结构），用于查询物品价格 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Config")
	TObjectPtr<UDataTable> ItemDataTable;

	/** 出售折扣系数：玩家出售物品实际收益 = SellPrice * SellPriceMultiplier */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Shop|Config", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float SellPriceMultiplier;

private:
	/** 获取指定玩家的库存组件 */
	UPP_InventoryComponent* GetPlayerInventory(APlayerController* PC) const;

	/** 在商品数据表中查找指定商品 */
	FPP_ShopItem* FindShopItem(FName ItemID) const;
};
