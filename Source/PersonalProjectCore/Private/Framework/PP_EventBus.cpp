// Chen Tianbao uses GAS to create online battle games

#include "Framework/PP_EventBus.h"

void UPP_EventBus::Unsubscribe(FDelegateHandle Handle)
{
	if (UScriptStruct** StructPtr = HandleToStruct.Find(Handle))
	{
		UScriptStruct* Struct = *StructPtr;
		if (TMap<FDelegateHandle, FEventCallback>* Callbacks = Subscribers.Find(Struct))
		{
			Callbacks->Remove(Handle);
			if (Callbacks->Num() == 0)
			{
				Subscribers.Remove(Struct);
			}
		}
		HandleToStruct.Remove(Handle);
	}
}

void UPP_EventBus::ClearAll()
{
	Subscribers.Reset();
	HandleToStruct.Reset();
}

void UPP_EventBus::Deinitialize()
{
	ClearAll();
	Super::Deinitialize();
}
