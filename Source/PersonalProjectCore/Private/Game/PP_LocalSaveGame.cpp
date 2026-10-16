// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_LocalSaveGame.h"
#include "Kismet/GameplayStatics.h"

UPP_LocalSaveGame* UPP_LocalSaveGame::LoadOrCreate()
{
	USaveGame* Loaded = UGameplayStatics::LoadGameFromSlot(GetSlotName(), 0);
	if (UPP_LocalSaveGame* Data = Cast<UPP_LocalSaveGame>(Loaded))
	{
		return Data;
	}

	// 没有存档或存档类型对不上（比如改过类名）：返回新建的
	return Cast<UPP_LocalSaveGame>(UGameplayStatics::CreateSaveGameObject(UPP_LocalSaveGame::StaticClass()));
}

void UPP_LocalSaveGame::Save(UPP_LocalSaveGame* SaveData)
{
	if (!SaveData) return;

	UGameplayStatics::SaveGameToSlot(SaveData, GetSlotName(), 0);
}
