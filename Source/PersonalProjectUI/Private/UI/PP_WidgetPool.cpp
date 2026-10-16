// Chen Tianbao uses GAS to create online battle games

#include "UI/PP_WidgetPool.h"

void UPP_WidgetPool::Initialize(UUserWidget* PoolOwner, TSubclassOf<UUserWidget> WidgetClass, int32 InitialSize)
{
	if (!IsValid(PoolOwner) || !WidgetClass)
	{
		return;
	}

	OwnerWidget = PoolOwner;
	PooledWidgetClass = WidgetClass;

	// 预创建Widget填充池
	for (int32 i = 0; i < InitialSize; ++i)
	{
		UUserWidget* NewWidget = CreateWidget<UUserWidget>(PoolOwner, WidgetClass);
		if (IsValid(NewWidget))
		{
			NewWidget->SetVisibility(ESlateVisibility::Hidden);
			Available.Add(NewWidget);
		}
	}
}

UUserWidget* UPP_WidgetPool::Acquire()
{
	UUserWidget* Widget = nullptr;

	// 优先从可用列表取
	if (Available.Num() > 0)
	{
		Widget = Available.Pop();
	}
	// 可用列表为空，新建一个
	else if (PooledWidgetClass && OwnerWidget.IsValid())
	{
		Widget = CreateWidget<UUserWidget>(OwnerWidget.Get(), PooledWidgetClass);
	}

	if (IsValid(Widget))
	{
		Widget->SetVisibility(ESlateVisibility::Visible);
		InUse.Add(Widget);
	}

	return Widget;
}

void UPP_WidgetPool::Release(UUserWidget* Widget)
{
	if (!IsValid(Widget))
	{
		return;
	}

	// 防重复归还：不在 InUse 中的 Widget 说明已被归还过（或不属于本池）。
	// 若不拦截，同一实例会在 Available 中出现两份，导致后续两次 Acquire 拿到同一对象。
	if (InUse.Remove(Widget) == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[WidgetPool] Release 忽略：%s 不在用列表中（重复归还或不属于本池）"), *Widget->GetName());
		return;
	}

	// 重置状态并归还可用列表
	Widget->RemoveFromParent();
	Widget->SetVisibility(ESlateVisibility::Hidden);
	Available.Add(Widget);
}

void UPP_WidgetPool::Empty()
{
	// 销毁所有在用Widget
	for (UUserWidget* Widget : InUse)
	{
		if (IsValid(Widget))
		{
			Widget->RemoveFromParent();
		}
	}
	InUse.Empty();

	// 销毁所有可用Widget
	for (UUserWidget* Widget : Available)
	{
		if (IsValid(Widget))
		{
			Widget->RemoveFromParent();
		}
	}
	Available.Empty();
}
