// 商店系统 — 商店动作接口
// UI 通过本接口发起购买/出售请求，具体实现转发到组件的 Server RPC。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PP_ShopActions.generated.h"

class APlayerController;

UINTERFACE(BlueprintType)
class UPP_ShopActions : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_ShopActions — 商店写操作能力接口
 * 实现方：UPP_ShopComponent
 * 调用方：UI 模块（数量选择弹窗确认购买/出售）
 *
 * 约定：这些方法均为「请求」，实现方内部转发到 Server RPC，由服务器权威执行。
 */
class PERSONALPROJECTINVENTORY_API IPP_ShopActions
{
	GENERATED_BODY()

public:
	/** 请求购买物品 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Actions")
	void RequestBuyItem(APlayerController* Buyer, FName ItemID, int32 Count);

	/** 请求出售物品 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Shop|Actions")
	void RequestSellItem(APlayerController* Seller, FName ItemID, int32 Count);
};
