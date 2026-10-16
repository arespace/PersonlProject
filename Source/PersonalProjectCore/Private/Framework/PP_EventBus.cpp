// Chen Tianbao uses GAS to create online battle games

#include "Framework/PP_EventBus.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

UPP_EventBus* UPP_EventBus::Get(const UObject* WorldContext)
{
	if (!WorldContext)
	{
		return nullptr;
	}
	const UWorld* World = WorldContext->GetWorld();
	UGameInstance* GI = World ? World->GetGameInstance() : nullptr;
	return GI ? GI->GetSubsystem<UPP_EventBus>() : nullptr;
}

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
