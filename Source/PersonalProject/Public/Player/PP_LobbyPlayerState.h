// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "Framework/PP_LobbyTypes.h"
#include "Framework/PP_LobbyPlayerInfo.h"
#include "PP_LobbyPlayerState.generated.h"

/**
 * APP_LobbyPlayerState - 大厅阶段玩家状态
 * 不继承IAbilitySystemInterface，大厅阶段不使用GAS
 * 复制Ready状态和角色选择到所有客户端
 */
UCLASS()
class PERSONALPROJECT_API APP_LobbyPlayerState : public APlayerState, public IPP_LobbyPlayerInfo
{
	GENERATED_BODY()

public:
	APP_LobbyPlayerState();

	// ===== IPP_LobbyPlayerInfo 接口实现（UI 大厅 Widget 经 Execute_ 读取）=====
	virtual bool IsLobbyHost_Implementation() override { return bIsHost; }
	virtual bool IsLobbyReady_Implementation() override { return bIsReady; }
	virtual int32 GetLobbyPlayerIndex_Implementation() override { return PlayerIndex; }

	// ===== Server RPC =====

	/** 设置准备状态 */
	UFUNCTION(Server, Reliable)
	void Server_SetReady(bool bReady);

	/** 选择角色 */
	UFUNCTION(Server, Reliable)
	void Server_SelectCharacter(FName CharacterId);

	// ===== 属性访问器 =====

	bool IsReady() const { return bIsReady; }
	FName GetSelectedCharacterId() const { return SelectedCharacterId; }
	int32 GetPlayerIndex() const { return PlayerIndex; }
	bool IsHost() const { return bIsHost; }
	void SetIsHost(bool bHost) { bIsHost = bHost; }

	/** 由LobbyGameMode调用设置玩家序号 */
	void SetPlayerIndex(int32 InIndex) { PlayerIndex = InIndex; }

protected:

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;

private:

	/** 是否准备 */
	UPROPERTY(ReplicatedUsing = OnRep_ReadyStatus)
	bool bIsReady = false;

	/** 选中角色ID */
	UPROPERTY(ReplicatedUsing = OnRep_SelectedCharacter)
	FName SelectedCharacterId;

	/** 玩家序号 */
	UPROPERTY(Replicated)
	int32 PlayerIndex = -1;

	/** 是否为房主 */
	UPROPERTY(ReplicatedUsing = OnRep_HostStatus)
	bool bIsHost = false;

	UFUNCTION()
	void OnRep_ReadyStatus();

	UFUNCTION()
	void OnRep_SelectedCharacter();

	UFUNCTION()
	void OnRep_HostStatus();
};