// Chen Tianbao uses GAS to create online battle games

#include "Player/PP_LobbyPlayerState.h"
#include "Game/PP_LobbyGameMode.h"
#include "Net/UnrealNetwork.h"

APP_LobbyPlayerState::APP_LobbyPlayerState()
{
	SetNetUpdateFrequency(30.f);
}

void APP_LobbyPlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_LobbyPlayerState, bIsReady);
	DOREPLIFETIME(APP_LobbyPlayerState, SelectedCharacterId);
	DOREPLIFETIME(APP_LobbyPlayerState, PlayerIndex);
	DOREPLIFETIME(APP_LobbyPlayerState, bIsHost);
}

void APP_LobbyPlayerState::Server_SetReady_Implementation(bool bReady)
{
	bIsReady = bReady;

	// Ready 状态变更后，广播更新的玩家列表给所有客户端
	APP_LobbyGameMode* LobbyGM = GetWorld() ? GetWorld()->GetAuthGameMode<APP_LobbyGameMode>() : nullptr;
	if (LobbyGM)
	{
		LobbyGM->BroadcastPlayerListUpdate();
	}
}

void APP_LobbyPlayerState::Server_SelectCharacter_Implementation(FName CharacterId)
{
	SelectedCharacterId = CharacterId;
}

void APP_LobbyPlayerState::OnRep_ReadyStatus()
{
	// RepNotify：客户端属性变更时触发，LobbyWidget通过RPC获取更新
}

void APP_LobbyPlayerState::OnRep_SelectedCharacter()
{
	// RepNotify：客户端角色选择变更时触发
}

void APP_LobbyPlayerState::OnRep_HostStatus()
{
	// RepNotify：客户端房主状态变更时触发
}