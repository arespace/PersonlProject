// UI系统 — UIManager子系统
// 管理所有屏幕空间Widget的层级栈、对象池、输入模式

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "PP_UIManager.generated.h"

class UPP_UIUserWidget;

/**
 * UI层级枚举
 * 值越大Z序越高，输入优先级越高
 */
UENUM(BlueprintType)
enum class EPP_UILayer : uint8
{
    Background   = 0   UMETA(DisplayName = "背景层"),
    Gameplay     = 10  UMETA(DisplayName = "游戏层"),
    Notification = 20  UMETA(DisplayName = "通知层"),
    Overlay      = 30  UMETA(DisplayName = "覆盖层"),
    Modal        = 40  UMETA(DisplayName = "模态层"),
    Debug        = 50  UMETA(DisplayName = "调试层")
};

/** Widget 推入事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWidgetPushedEvent, UPP_UIUserWidget*, Widget, EPP_UILayer, Layer);

/** Widget 弹出事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnWidgetPoppedEvent, UPP_UIUserWidget*, Widget, EPP_UILayer, Layer);

/** 输入模式变化事件 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnInputModeChangedEvent, FName, NewMode);

/**
 * UPP_UIManager — GameInstanceSubsystem
 *
 * 职责：
 *   1. 维护6层 Widget 栈（Background → Debug），管理Z序
 *   2. Widget对象池（高频创建/销毁的浮层控件复用）
 *   3. 输入模式栈（感知Widget push/pop自动切换Game/UI/GameAndUI）
 *   4. 广播层级变化事件（OnWidgetPushed/Popped, OnInputModeChanged）
 *
 * 单例：通过 GetGameInstance()->GetSubsystem<UPP_UIManager>() 获取
 */
UCLASS()
class PERSONALPROJECT_API UPP_UIManager : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    // ===== Widget 层级管理 =====

    /**
     * 将 Widget 推入指定层级栈顶
     * 触发 OnWidgetPushed 回调 → 重新计算输入模式
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Manager")
    void PushWidget(UPP_UIUserWidget* Widget, EPP_UILayer Layer);

    /**
     * 将 Widget 从层级栈弹出
     * 触发 OnWidgetPopped 回调 → 播放退场动画 → RemoveFromParent → 重算输入模式
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Manager")
    void PopWidget(UPP_UIUserWidget* Widget);

    /** 获取指定层级最顶部的 Widget，无则返回 nullptr */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI|Manager")
    UPP_UIUserWidget* GetTopWidgetInLayer(EPP_UILayer Layer) const;

    /** 判断指定 Widget 是否在栈中 */
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "UI|Manager")
    bool IsWidgetInStack(UPP_UIUserWidget* Widget) const;

    // ===== Widget 对象池 =====

    /**
     * 从对象池获取 Widget（不存在则 CreateWidget）
     * @param WidgetClass Widget 类
     * @return 可用的 Widget 实例
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Pool")
    UUserWidget* AcquireFromPool(TSubclassOf<UUserWidget> WidgetClass);

    /**
     * 归还 Widget 到对象池（隐藏并缓存，不销毁）
     * @param Widget 要归还的 Widget
     */
    UFUNCTION(BlueprintCallable, Category = "UI|Pool")
    void ReleaseToPool(UUserWidget* Widget);

    // ===== 输入模式管理 =====

    /** 切换到纯游戏输入模式 */
    UFUNCTION(BlueprintCallable, Category = "UI|Input")
    void SetInputModeGameplay();

    /** 切换到纯UI输入模式 */
    UFUNCTION(BlueprintCallable, Category = "UI|Input")
    void SetInputModeUI();

    /** 切换到游戏+UI混合输入模式 */
    UFUNCTION(BlueprintCallable, Category = "UI|Input")
    void SetInputModeGameAndUI();

    // ===== 委托 =====

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnWidgetPushedEvent OnWidgetPushed;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnWidgetPoppedEvent OnWidgetPopped;

    UPROPERTY(BlueprintAssignable, Category = "UI|Events")
    FOnInputModeChangedEvent OnInputModeChanged;

private:
    /** 应用当前应使用的输入模式（从顶部到底部扫描 bConsumesAllInput） */
    void RecalculateInputMode();

    /** 获取当前有效的 PlayerController */
    APlayerController* GetPlayerController() const;

    // ===== 数据 =====

    /** 每层 Widget 栈：Layer → 该层的 Widget 列表（栈顶在末尾） */
    TMap<EPP_UILayer, TArray<TWeakObjectPtr<UPP_UIUserWidget>>> LayerStacks;

    /** Widget → Layer 反向索引，O(1) 查找 Widget 所在层级 */
    TMap<TWeakObjectPtr<UPP_UIUserWidget>, EPP_UILayer> WidgetToLayer;

    /** Widget 对象池：WidgetClass → 已缓存但未显示的 Widget 实例 */
    TMap<TSubclassOf<UUserWidget>, TArray<TObjectPtr<UUserWidget>>> WidgetPool;

    /** 当前激活的输入模式名称（Gameplay/UI/GameAndUI） */
    FName CurrentInputMode;
};
