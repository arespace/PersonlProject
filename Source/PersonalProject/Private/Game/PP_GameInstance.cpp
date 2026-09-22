// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_GameInstance.h"
#include "Engine/World.h"

UPP_GameInstance* UPP_GameInstance::Get(const UWorld* World)
{
	if (!World) return nullptr;
	return Cast<UPP_GameInstance>(World->GetGameInstance());
}

void UPP_GameInstance::SetPlayerCharacterSelection(const FString& PlayerName, FName CharacterId)
{
	if (PlayerName.IsEmpty() || CharacterId.IsNone()) return;

	PlayerCharacterSelections.Add(PlayerName, CharacterId);
	LastCharacterId = CharacterId;  // 后备方案：不依赖PlayerName匹配
	UE_LOG(LogTemp, Log, TEXT("[GameInstance] SetPlayerCharacterSelection - Player: %s, Character: %s"),
		*PlayerName, *CharacterId.ToString());
}

FName UPP_GameInstance::GetPlayerCharacterSelection(const FString& PlayerName) const
{
	if (const FName* Found = PlayerCharacterSelections.Find(PlayerName))
	{
		UE_LOG(LogTemp, Log, TEXT("[GameInstance] GetPlayerCharacterSelection - Player: %s, Character: %s"),
			*PlayerName, *(*Found).ToString());
		return *Found;
	}
	return NAME_None;
}

void UPP_GameInstance::ClearPlayerCharacterSelection(const FString& PlayerName)
{
	PlayerCharacterSelections.Remove(PlayerName);
}

void UPP_GameInstance::ClearAllPlayerCharacterSelections()
{
	PlayerCharacterSelections.Empty();
}
