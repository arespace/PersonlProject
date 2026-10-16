// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_GameInstance.h"
#include "Engine/World.h"

UPP_GameInstance* UPP_GameInstance::Get(const UWorld* World)
{
	if (!World) return nullptr;
	return Cast<UPP_GameInstance>(World->GetGameInstance());
}
