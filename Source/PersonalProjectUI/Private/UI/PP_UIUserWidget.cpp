// UI系统 — 屏幕空间Widget基类实现

#include "UI/PP_UIUserWidget.h"
#include "UI/PP_UIManager.h"
#include "Engine/GameInstance.h"

void UPP_UIUserWidget::NativeConstruct()
{
    Super::NativeConstruct();

    // 自动向 UIManager 注册
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UPP_UIManager* UIManager = GI->GetSubsystem<UPP_UIManager>())
        {
            UIManager->PushWidget(this, OwningLayer);
        }
    }
}

void UPP_UIUserWidget::NativeDestruct()
{
    // 从 UIManager 注销（仅当仍在栈中时）
    if (UGameInstance* GI = GetGameInstance())
    {
        if (UPP_UIManager* UIManager = GI->GetSubsystem<UPP_UIManager>())
        {
            if (UIManager->IsWidgetInStack(this))
            {
                UIManager->PopWidget(this);
            }
        }
    }

    Super::NativeDestruct();
}
