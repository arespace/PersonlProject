// 商店系统 — HUD金币常驻显示控件
// 通过 EventBus 的 FPP_GoldUpdatedEvent 实时更新

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/PP_EventTypes.h"
#include "PP_GoldDisplayWidget.generated.h"

class UTextBlock;

/**
 * 金币HUD显示Widget
 *
 * 职责：
 *   1. 在HUD上常驻显示当前金币数
 *   2. 监听 EventBus 的 FPP_GoldUpdatedEvent 实时刷新文本
 *   3. 提供 UpdateGoldDisplay 方法供外部主动刷新
 *
 * 使用约定：由 APP_HUD 创建并加入 ZOrder_CombatHUD 层
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_GoldDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 绑定到指定的库存数据源
	 * 自动订阅 EventBus 的金币变更事件并立即刷新一次显示
	 * @param InInventory 实现 IPP_InventoryQuery 的对象
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void BindInventoryComponent(UObject* InInventory);

	/**
	 * 主动刷新金币显示（外部强制更新场景）
	 * @param NewGold 当前金币数
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void UpdateGoldDisplay(int32 NewGold);

protected:
	virtual void NativeDestruct() override;

	/** 金币数显示文本 */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> GoldText;

private:
	/** 当前绑定的库存数据源 */
	UPROPERTY()
	TWeakObjectPtr<UObject> Inventory;

	/** EventBus 订阅句柄 */
	FDelegateHandle GoldUpdatedHandle;

	void HandleGoldUpdated(const FPP_GoldUpdatedEvent& Event);
};
