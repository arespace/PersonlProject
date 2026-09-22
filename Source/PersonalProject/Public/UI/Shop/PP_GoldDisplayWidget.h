// 商店系统 — HUD金币常驻显示控件
// 监听 InventoryComponent.OnGoldChanged 实时更新

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_GoldDisplayWidget.generated.h"

class UPP_InventoryComponent;
class UTextBlock;

/**
 * 金币HUD显示Widget
 *
 * 职责：
 *   1. 在HUD上常驻显示当前金币数
 *   2. 监听 InventoryComponent::OnGoldChanged 实时刷新文本
 *   3. 提供 UpdateGoldDisplay 方法供外部主动刷新
 *
 * 使用约定：由 APP_HUD 创建并加入 ZOrder_CombatHUD 层
 */
UCLASS()
class PERSONALPROJECT_API UPP_GoldDisplayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * 绑定到指定的库存组件
	 * 自动监听 OnGoldChanged 并立即刷新一次显示
	 */
	UFUNCTION(BlueprintCallable, Category = "Shop|UI")
	void BindInventoryComponent(UPP_InventoryComponent* InInventory);

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
	/** 当前绑定的库存组件 */
	UPROPERTY()
	TObjectPtr<UPP_InventoryComponent> Inventory;

	UFUNCTION()
	void HandleGoldChanged(int32 NewGold);
};
