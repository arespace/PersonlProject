// 物品提示框控件 — 鼠标悬停显示物品详细信息 + 装备属性对比

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "Inventory/Data/PP_ItemData.h"
#include "Utils/PP_BlueprintLibrary.h"
#include "PP_ItemTooltipWidget.generated.h"

class UImage;
class UTextBlock;
class UVerticalBox;
class UPP_InventoryComponent;

/**
 * 物品提示框 Widget
 *
 * 显示内容：
 *   - 图标 + 名称（稀有度配色）
 *   - 类型标签 + 装备槽位
 *   - 属性修改器列表（来自 EquipEffectClass）
 *   - 描述文本
 *   - 价格 + 堆叠信息
 *   - 若悬停装备且对应槽位有装备：显示对比
 *
 * OwningLayer = Overlay, bConsumesAllInput = false（不阻挡点击）
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECT_API UPP_ItemTooltipWidget : public UPP_UIUserWidget
{
    GENERATED_BODY()

public:
    UPP_ItemTooltipWidget();

    /**
     * 配置提示框内容
     * @param InInventory    库存组件引用（用于查询已装备物品做对比）
     * @param InItemInfo     物品配置数据
     * @param InStackCount   当前堆叠数量（0表示不显示）
     * @param InMaxStack     最大堆叠数（0表示不显示）
     */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void SetupTooltip(UPP_InventoryComponent* InInventory,
                      const FPP_ItemInfo& InItemInfo,
                      int32 InStackCount = 0,
                      int32 InMaxStack = 0);

    /** 更新提示框位置（跟随鼠标） */
    UFUNCTION(BlueprintCallable, Category = "Tooltip")
    void UpdatePosition(FVector2D MousePosition);

protected:
    virtual void NativeConstruct() override;

    // ===== 蓝图绑定控件（BindWidgetOptional） =====

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UImage> ItemIcon;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> NameText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> TypeText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> DescriptionText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> PriceText;

    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UTextBlock> StackText;

    /** 属性列表容器（VerticalBox），每行一个属性 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> StatsContainer;

    /** 对比区域容器（VerticalBox），仅在装备对比时显示 */
    UPROPERTY(BlueprintReadOnly, meta = (BindWidgetOptional))
    TObjectPtr<UVerticalBox> CompareContainer;

    /** 蓝图可重写：设置名称颜色（根据稀有度） */
    UFUNCTION(BlueprintImplementableEvent, Category = "Tooltip")
    void ApplyRarityColor(EPP_ItemRarity Rarity);

private:
    /** 构建属性列表 */
    void BuildStatsList(const TArray<FPP_ModifierSummary>& Modifiers);

    /** 构建装备对比区域 */
    void BuildComparison(UPP_InventoryComponent* Inventory, const FPP_ItemInfo& HoveredInfo);

    /** 缓存的物品配置 */
    FPP_ItemInfo CachedInfo;
};
