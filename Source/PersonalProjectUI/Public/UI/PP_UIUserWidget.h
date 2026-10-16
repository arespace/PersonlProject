// UI系统 — 屏幕空间Widget基类
// 提供层级集成、生命周期回调和UIManager自动注册

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/PP_UIManager.h"
#include "PP_UIUserWidget.generated.h"

/**
 * UPP_UIUserWidget — 所有屏幕空间面板的基类
 *
 * 使用约定：
 *   - 所有独立屏幕空间面板必须继承此类
 *   - 蓝图子类需设置 OwningLayer
 *   - 面板内嵌的子控件（如物品格子）不继承此类，仍用 UUserWidget
 *   - 重写生命周期蓝图事件以添加自定义入场/退场动画
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class PERSONALPROJECTUI_API UPP_UIUserWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    // ===== 配置属性（蓝图可编辑） =====

    /** 所属UI层级，决定Z序和输入优先级 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Config")
    EPP_UILayer OwningLayer = EPP_UILayer::Modal;

    /** 是否消费全部输入（为 true 时，当此Widget为最顶层，输入模式切换为UI模式） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Input")
    bool bConsumesAllInput = true;

    /** 是否启用入场/退场动画 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI|Animation")
    bool bUseEntranceExitAnimations = true;

    // ===== 查询 =====

    bool DoesConsumeAllInput() const { return bConsumesAllInput; }
    EPP_UILayer GetOwningLayer() const { return OwningLayer; }

    // ===== 生命周期（BlueprintNativeEvent，蓝图可重写） =====

    /** Widget已被推入层级栈（播放入场动画的时机） */
    UFUNCTION(BlueprintNativeEvent, Category = "UI|Lifecycle")
    void OnWidgetPushed();
    virtual void OnWidgetPushed_Implementation() {}

    /** Widget即将被弹出（播放退场动画的时机） */
    UFUNCTION(BlueprintNativeEvent, Category = "UI|Lifecycle")
    void OnWidgetPopped();
    virtual void OnWidgetPopped_Implementation() {}

    /** Widget被更高层的Widget遮挡（可暂停Tick/动画） */
    UFUNCTION(BlueprintNativeEvent, Category = "UI|Lifecycle")
    void OnWidgetCovered();
    virtual void OnWidgetCovered_Implementation() {}

    /** Widget重新成为该层最顶层（可恢复Tick/动画） */
    UFUNCTION(BlueprintNativeEvent, Category = "UI|Lifecycle")
    void OnWidgetRevealed();
    virtual void OnWidgetRevealed_Implementation() {}

protected:
    virtual void NativeConstruct() override;
    virtual void NativeDestruct() override;
};
