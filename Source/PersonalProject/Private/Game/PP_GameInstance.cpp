// Chen Tianbao uses GAS to create online battle games

#include "Game/PP_GameInstance.h"
#include "Framework/PP_SessionManager.h"
#include "Engine/World.h"

UPP_GameInstance* UPP_GameInstance::Get(const UWorld* World)
{
	if (!World) return nullptr;
	return Cast<UPP_GameInstance>(World->GetGameInstance());
}

void UPP_GameInstance::SetPlayerCharacterSelection(const FString& PlayerName, FName CharacterId)
{
	if (UPP_SessionManager* SM = GetSubsystem<UPP_SessionManager>())
	{
		SM->SetPlayerCharacterSelection(PlayerName, CharacterId);
	}
}

FName UPP_GameInstance::GetPlayerCharacterSelection(const FString& PlayerName) const
{
	if (UPP_SessionManager* SM = GetSubsystem<UPP_SessionManager>())
	{
		return SM->GetPlayerCharacterSelection(PlayerName);
	}
	return NAME_None;
}

FName UPP_GameInstance::GetLastCharacterSelection() const
{
	if (UPP_SessionManager* SM = GetSubsystem<UPP_SessionManager>())
	{
		return SM->GetLastCharacterSelection();
	}
	return NAME_None;
}

void UPP_GameInstance::ClearPlayerCharacterSelection(const FString& PlayerName)
{
	if (UPP_SessionManager* SM = GetSubsystem<UPP_SessionManager>())
	{
		SM->ClearPlayerCharacterSelection(PlayerName);
	}
}

void UPP_GameInstance::ClearAllPlayerCharacterSelections()
{
	if (UPP_SessionManager* SM = GetSubsystem<UPP_SessionManager>())
	{
		SM->ClearAllPlayerCharacterSelections();
	}
}
