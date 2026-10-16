// UI系统 — 屏幕空间Widget基类实现

#include "Framework/PP_UIUserWidget.h"
#include "Framework/PP_UIManager.h"
#include "Engine/GameInstance.h"

void UPP_UIUserWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 自动向 UIManager 注册
    if (UPP_UIManager* UIManager = UPP_UIManager::Get(this))
    {
        UIManager->PushWidget(this, OwningLayer);
    }
}

void UPP_UIUserWidget::NativeDestruct()
{
    // 统一退订托管的 EventBus 订阅
    UnsubscribeEventBus();

    // 从 UIManager 注销（仅当仍在栈中时）
    if (UPP_UIManager* UIManager = UPP_UIManager::Get(this))
    {
        if (UIManager->IsWidgetInStack(this))
        {
            UIManager->PopWidget(this);
        }
    }

    Super::NativeDestruct();
}

void UPP_UIUserWidget::UnsubscribeEventBus()
{
    if (EventBusHandles.Num() > 0)
    {
        if (UPP_EventBus* Bus = UPP_EventBus::Get(this))
        {
            for (const FDelegateHandle& Handle : EventBusHandles)
            {
                Bus->Unsubscribe(Handle);
            }
        }
        EventBusHandles.Empty();
    }
}
