// UI系统 — Widget 层级枚举
// 独立轻量头文件：仅需要层级枚举的类型不必再包含 UIManager 子系统头

#pragma once

#include "CoreMinimal.h"
#include "PP_UILayerTypes.generated.h"

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
