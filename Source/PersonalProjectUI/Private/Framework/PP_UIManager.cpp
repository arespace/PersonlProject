// UI系统 — UIManager子系统实现

#include "Framework/PP_UIManager.h"
#include "Framework/PP_UIUserWidget.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/GameInstance.h"
#include "Blueprint/UserWidget.h"
#include "Framework/Application/SlateApplication.h"
#include "Engine/World.h"

UPP_UIManager* UPP_UIManager::Get(const UObject* WorldContext)
{
    if (!WorldContext)
    {
        return nullptr;
    }
    const UWorld* World = WorldContext->GetWorld();
    UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
    return GI ? GI->GetSubsystem<UPP_UIManager>() : nullptr;
}

void UPP_UIManager::PushWidget(UPP_UIUserWidget* Widget, EPP_UILayer Layer)
{
    if (!IsValid(Widget))
    {
        return;
    }

    TArray<TWeakObjectPtr<UPP_UIUserWidget>>& Stack = LayerStacks.FindOrAdd(Layer);

    // O(1) 反向索引检查：避免重复推入
    if (WidgetToLayer.Contains(Widget))
    {
        return;
    }

    Stack.Add(Widget);
    WidgetToLayer.Add(Widget, Layer);
    Widget->OnWidgetPushed();
    OnWidgetPushed.Broadcast(Widget, Layer);

    // 通知下层Widget被覆盖
    if (Stack.Num() > 1)
    {
        UPP_UIUserWidget* Covered = Stack[Stack.Num() - 2].Get();
        if (Covered)
        {
            Covered->OnWidgetCovered();
        }
    }

    RecalculateInputMode();
}

void UPP_UIManager::PopWidget(UPP_UIUserWidget* Widget)
{
    if (!IsValid(Widget))
    {
        return;
    }

    // O(1) 反向索引查找 Widget 所在层级
    const EPP_UILayer* FoundLayerPtr = WidgetToLayer.Find(Widget);
    if (!FoundLayerPtr)
    {
        return;
    }
    const EPP_UILayer FoundLayer = *FoundLayerPtr;

    // 从层级栈中移除（保持顺序不变）
    TArray<TWeakObjectPtr<UPP_UIUserWidget>>* StackPtr = LayerStacks.Find(FoundLayer);
    if (StackPtr)
    {
        const int32 Index = StackPtr->IndexOfByPredicate([Widget](const TWeakObjectPtr<UPP_UIUserWidget>& W) {
            return W.Get() == Widget;
        });
        if (Index != INDEX_NONE)
        {
            StackPtr->RemoveAt(Index);
        }
    }

    WidgetToLayer.Remove(Widget);

    Widget->OnWidgetPopped();
    OnWidgetPopped.Broadcast(Widget, FoundLayer);

    // 通知该层新的栈顶Widget被揭示
    if (StackPtr && StackPtr->Num() > 0)
    {
        UPP_UIUserWidget* Revealed = StackPtr->Last().Get();
        if (Revealed)
        {
            Revealed->OnWidgetRevealed();
        }
    }

    Widget->RemoveFromParent();
    RecalculateInputMode();
}

UPP_UIUserWidget* UPP_UIManager::GetTopWidgetInLayer(EPP_UILayer Layer) const
{
    const TArray<TWeakObjectPtr<UPP_UIUserWidget>>* Stack = LayerStacks.Find(Layer);
    if (Stack && Stack->Num() > 0)
    {
        return Stack->Last().Get();
    }
    return nullptr;
}

bool UPP_UIManager::IsWidgetInStack(UPP_UIUserWidget* Widget) const
{
    if (!IsValid(Widget)) return false;
    return WidgetToLayer.Contains(Widget);
}

void UPP_UIManager::SetInputModeGameplay()
{
    if (APlayerController* PC = GetPlayerController())
    {
        // 强制清零 ignore 计数器（SetIgnoreMoveInput(false) 是递减，若之前被多次加过会残留）
        PC->ResetIgnoreInputFlags();
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;

        // 显式将 Slate 焦点交还给 GameViewport，防止 RemoveFromParent 后 Slate 仍持有捕获
        FSlateApplication::Get().SetUserFocusToGameViewport(0);
        FSlateApplication::Get().ReleaseAllPointerCapture();

        CurrentInputMode = TEXT("Gameplay");
        OnInputModeChanged.Broadcast(CurrentInputMode);
    }
}

void UPP_UIManager::SetInputModeUI()
{
    if (APlayerController* PC = GetPlayerController())
    {
        // 停住角色并禁止移动/视角输入，防止 UIOnly 模式下按键释放事件丢失导致角色继续移动
        PC->SetIgnoreMoveInput(true);
        PC->SetIgnoreLookInput(true);
        if (APawn* Pawn = PC->GetPawn())
        {
            if (UCharacterMovementComponent* CMC = Pawn->FindComponentByClass<UCharacterMovementComponent>())
            {
                CMC->StopMovementImmediately();
            }
        }

        PC->SetInputMode(FInputModeUIOnly());
        PC->bShowMouseCursor = true;
        CurrentInputMode = TEXT("UI");
        OnInputModeChanged.Broadcast(CurrentInputMode);
    }
}

void UPP_UIManager::SetInputModeGameAndUI()
{
    if (APlayerController* PC = GetPlayerController())
    {
        PC->SetIgnoreMoveInput(false);
        PC->SetIgnoreLookInput(false);
        FInputModeGameAndUI InputMode;
        InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(InputMode);
        PC->bShowMouseCursor = true;
        CurrentInputMode = TEXT("GameAndUI");
        OnInputModeChanged.Broadcast(CurrentInputMode);
    }
}

void UPP_UIManager::RefreshInputMode()
{
    RecalculateInputMode();
}

void UPP_UIManager::RecalculateInputMode()
{
    // 只扫描 Modal 和 Overlay 层决定输入模式
    // Notification / Gameplay / Background 层的 HUD 等不应影响输入模式切换
    constexpr EPP_UILayer Layers[] = {
        EPP_UILayer::Modal,
        EPP_UILayer::Overlay,
    };

    for (EPP_UILayer Layer : Layers)
    {
        UPP_UIUserWidget* Top = GetTopWidgetInLayer(Layer);
        if (Top && Top->DoesConsumeAllInput() && Top->IsVisible())
        {
            if (Layer == EPP_UILayer::Modal)
            {
                SetInputModeUI();
            }
            else
            {
                SetInputModeGameAndUI();
            }
            return;
        }
    }

    // 没有任何 Modal/Overlay Widget 消费输入 → 纯游戏
    SetInputModeGameplay();
}

APlayerController* UPP_UIManager::GetPlayerController() const
{
    UGameInstance* GI = GetGameInstance();
    if (!GI) return nullptr;

    UWorld* World = GI->GetWorld();
    if (!World) return nullptr;

    return World->GetFirstPlayerController();
}
