// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Delegates/Delegate.h"
#include "Templates/Function.h"
#include "PP_EventBus.generated.h"

class UScriptStruct;

/**
 * 事件总线：GameInstance 级别子系统，用于系统间解耦通信。
 *
 * 用法（C++）：
 *   获取：UPP_EventBus* Bus = UPP_EventBus::Get(WorldContext);
 *   订阅：FDelegateHandle Handle = Bus->Subscribe<FEventType>(Callback);
 *   发布：UPP_EventBus::BroadcastEvent(WorldContext, FEventType{...});  // 或拿到 Bus 后 Bus->Broadcast(Event)
 *   取消：Bus->Unsubscribe(Handle);
 *
 * 注意：订阅者必须在自身生命周期结束时（EndPlay / Deinitialize / NativeDestruct）调用 Unsubscribe，
 *      否则回调中访问已回收对象会导致崩溃。
 */
UCLASS()
class PERSONALPROJECTCORE_API UPP_EventBus : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	/**
	 * 从任意世界上下文获取事件总线
	 * 封装 GetWorld→GetGameInstance→GetSubsystem 三连判空，全工程统一入口；
	 * WorldContext 为空或无 World 时返回 nullptr（调用方判空后使用）。
	 */
	static UPP_EventBus* Get(const UObject* WorldContext);

	/** 发布事件到全局事件总线（内部经 Get 取总线，取不到则静默丢弃，简化广播点样板） */
	template<typename EventType>
	static void BroadcastEvent(const UObject* WorldContext, const EventType& Event);

	/** 订阅指定类型的事件，返回句柄用于取消订阅 */
	template<typename EventType>
	FDelegateHandle Subscribe(TFunction<void(const EventType&)> Callback);

	/** 发布事件，所有订阅者会收到通知 */
	template<typename EventType>
	void Broadcast(const EventType& Event);

	/** 取消订阅 */
	void Unsubscribe(FDelegateHandle Handle);

	/** 清除所有订阅（通常不需要手动调用，Subsystem 销毁时自动清理） */
	void ClearAll();

protected:
	virtual void Deinitialize() override;

private:
	/** 类型擦除后的回调包装 */
	struct FEventCallback
	{
		TFunction<void(const void*)> Callback;
	};

	/** 事件类型 -> (句柄 -> 回调) */
	TMap<UScriptStruct*, TMap<FDelegateHandle, FEventCallback>> Subscribers;

	/** 句柄 -> 事件类型，用于快速取消订阅 */
	TMap<FDelegateHandle, UScriptStruct*> HandleToStruct;
};

template<typename EventType>
void UPP_EventBus::BroadcastEvent(const UObject* WorldContext, const EventType& Event)
{
	if (UPP_EventBus* Bus = Get(WorldContext))
	{
		Bus->Broadcast(Event);
	}
}

template<typename EventType>
FDelegateHandle UPP_EventBus::Subscribe(TFunction<void(const EventType&)> Callback)
{
	UScriptStruct* Struct = EventType::StaticStruct();
	FDelegateHandle Handle(FDelegateHandle::GenerateNewHandle);

	FEventCallback Wrapped;
	Wrapped.Callback = [Callback = MoveTemp(Callback)](const void* Data)
	{
		Callback(*static_cast<const EventType*>(Data));
	};

	Subscribers.FindOrAdd(Struct).Add(Handle, MoveTemp(Wrapped));
	HandleToStruct.Add(Handle, Struct);
	return Handle;
}

template<typename EventType>
void UPP_EventBus::Broadcast(const EventType& Event)
{
	UScriptStruct* Struct = EventType::StaticStruct();
	if (TMap<FDelegateHandle, FEventCallback>* Callbacks = Subscribers.Find(Struct))
	{
		// 拷贝回调列表，避免回调中修改 Subscribers 导致迭代器失效
		TArray<FEventCallback> Snapshot;
		Snapshot.Reserve(Callbacks->Num());
		for (const auto& Pair : *Callbacks)
		{
			Snapshot.Add(Pair.Value);
		}

		for (const FEventCallback& Cb : Snapshot)
		{
			if (Cb.Callback)
			{
				Cb.Callback(&Event);
			}
		}
	}
}
