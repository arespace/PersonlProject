// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_WidgetPool.generated.h"

/**
 * UPP_WidgetPool - 通用Widget对象池
 * 复用频繁创建/销毁的Widget（如伤害数字、通知消息），避免GC压力
 * 使用方式：Initialize()初始化 -> Acquire()获取 -> Release()归还
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_WidgetPool : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化对象池，预创建指定数量的Widget */
	void Initialize(UUserWidget* PoolOwner, TSubclassOf<UUserWidget> WidgetClass, int32 InitialSize = 8);

	/** 从池中获取一个Widget，如果池为空则新建 */
	UUserWidget* Acquire();

	/** 将Widget归还到池中，移出视口并重置状态 */
	void Release(UUserWidget* Widget);

	/** 清空对象池，销毁所有Widget */
	void Empty();

	/** 获取当前可用数量 */
	int32 GetAvailableCount() const { return Available.Num(); }

	/** 获取当前在用数量 */
	int32 GetInUseCount() const { return InUse.Num(); }

private:
	/** 可用Widget列表 */
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> Available;

	/** 正在使用的Widget列表 */
	UPROPERTY()
	TArray<TObjectPtr<UUserWidget>> InUse;

	/** 池中Widget的类类型 */
	UPROPERTY()
	TSubclassOf<UUserWidget> PooledWidgetClass;

	TWeakObjectPtr<UUserWidget> OwnerWidget;
	/** 拥有此池的Widget，用于创建子Widget */
};
