// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Player/PP_LobbyPlayerState.h"
#include "PP_SessionManager.generated.h"

/** 房主迁移数据（存储在 GameInstance 中跨关卡持久化） */
USTRUCT(BlueprintType)
struct FHostMigrationData
{
	GENERATED_BODY()

	/** 是否有待处理的迁移 */
	UPROPERTY()
	bool bPendingMigration = false;

	/** 本玩家是否是新房主 */
	UPROPERTY()
	bool bIsNewHost = false;

	/** 本玩家的名称 */
	UPROPERTY()
	FString PlayerName;

	/** 新房主的名称（客户端用于匹配 Session） */
	UPROPERTY()
	FString NewHostPlayerName;

	/** 保存的房间设置 */
	UPROPERTY()
	FRoomSettings SavedRoomSettings;

	/** 最大玩家数 */
	UPROPERTY()
	int32 MaxPlayers = 4;
};

/** 房间信息（供UI显示） */
USTRUCT(BlueprintType)
struct FRoomInfo
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly)
	FString RoomName;  // 房主名

	UPROPERTY(BlueprintReadOnly)
	int32 CurrentPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 MaxPlayers = 0;

	UPROPERTY(BlueprintReadOnly)
	int32 Ping = 0;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPSessionsFound, const TArray<FRoomInfo>&, Rooms);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPCreateSessionComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPJoinSessionComplete, bool, bSuccess);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPPDestroySessionComplete, bool, bSuccess);

/**
 * UPP_SessionManager - LAN 会话管理器 (GameInstanceSubsystem)
 * 封装 Online Subsystem 的 Session 操作，提供创建/查找/加入房间功能
 * 生命周期跟随 GameInstance，跨关卡切换不会丢失
 */
UCLASS()
class PERSONALPROJECT_API UPP_SessionManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// --- Subsystem Lifecycle ---
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	/** 便捷静态方法：从 World 获取 SessionManager 实例 */
	static UPP_SessionManager* Get(const UWorld* World);

	/** 创建房间（Host调用） */
	UFUNCTION(BlueprintCallable, Category = "PP|Session")
	void CreateSession(const FString& PlayerName, int32 MaxPlayers = 4);

	/** 查找局域网房间 */
	UFUNCTION(BlueprintCallable, Category = "PP|Session")
	void FindSessions();

	/** 加入指定索引的房间 */
	UFUNCTION(BlueprintCallable, Category = "PP|Session")
	void JoinSession(int32 SessionIndex, const FString& PlayerName = TEXT("Player"));

	/** 销毁当前会话 */
	UFUNCTION(BlueprintCallable, Category = "PP|Session")
	void DestroyCurrentSession();

	// ===== 房主迁移 =====

	/** 设置迁移数据 */
	void SetMigrationData(const FHostMigrationData& Data);

	/** 获取迁移数据 */
	const FHostMigrationData& GetMigrationData() const { return PendingMigration; }

	/** 清除迁移数据 */
	void ClearMigrationData();

	/** 是否有待处理的迁移 */
	bool HasPendingMigration() const { return PendingMigration.bPendingMigration; }

	// ===== 角色选择桥接（大厅→战斗关卡） =====

	/** 存入玩家的角色选择（大厅阶段调用） */
	void SetPlayerCharacterSelection(const FString& PlayerName, FName CharacterId);

	/** 读取玩家的角色选择（不清除，可多次读取） */
	FName GetPlayerCharacterSelection(const FString& PlayerName) const;

	/** 取出玩家的角色选择（取出后清除） */
	FName ConsumePlayerCharacterSelection(const FString& PlayerName);

	/** 不依赖 PlayerName，返回最后存入的角色选择 */
	FName GetLastCharacterSelection() const { return LastCharacterId; }

	/** 清除指定玩家的角色选择 */
	void ClearPlayerCharacterSelection(const FString& PlayerName);

	/** 清除所有玩家的角色选择 */
	void ClearAllPlayerCharacterSelections();

	/** 新房主自动创建 Session 并开启 Listen Server */
	void ExecuteHostMigration_AsNewHost();

	/** 客户端自动搜索并加入新房主的 Session */
	void ExecuteHostMigration_AsClient();

	/** 查找结果委托 */
	UPROPERTY(BlueprintAssignable, Category = "PP|Session")
	FOnPPSessionsFound OnSessionsFound;

	/** 创建完成委托 */
	UPROPERTY(BlueprintAssignable, Category = "PP|Session")
	FOnPPCreateSessionComplete OnCreateSessionComplete;

	/** 加入完成委托 */
	UPROPERTY(BlueprintAssignable, Category = "PP|Session")
	FOnPPJoinSessionComplete OnJoinSessionComplete;

	/** 销毁完成委托 */
	UPROPERTY(BlueprintAssignable, Category = "PP|Session")
	FOnPPDestroySessionComplete OnDestroySessionComplete;

	/** 获取最近一次搜索的结果 */
	const TArray<FRoomInfo>& GetLastSearchResults() const { return LastSearchResults; }

private:
	IOnlineSessionPtr GetSessionInterface() const;

	void OnCreateSessionCompleteInternal(FName SessionName, bool bSuccess);
	void OnFindSessionsCompleteInternal(bool bSuccess);
	void OnJoinSessionCompleteInternal(FName SessionName, EOnJoinSessionCompleteResult::Type Result);
	void OnDestroySessionCompleteInternal(FName SessionName, bool bSuccess);

	TSharedPtr<FOnlineSessionSearch> SearchSettings;
	TArray<FRoomInfo> LastSearchResults;

	// 回调句柄
	FDelegateHandle CreateSessionDelegateHandle;
	FDelegateHandle FindSessionsDelegateHandle;
	FDelegateHandle JoinSessionDelegateHandle;
	FDelegateHandle DestroySessionDelegateHandle;

	// 创建完成后执行 ServerTravel 的数据
	FString PendingPlayerName;

	// JoinSession 时保存的玩家名（用于 ClientTravel URL）
	FString JoinPlayerName;

	// ===== 房主迁移内部 =====

	/** 待处理的迁移数据 */
	FHostMigrationData PendingMigration;

	/** 玩家角色选择缓存：PlayerName → CharacterId */
	TMap<FString, FName> PlayerCharacterSelections;

	/** 最后存入的角色选择（不依赖 PlayerName 匹配的后备方案） */
	FName LastCharacterId;

	/** 迁移创建 Session 完成回调 */
	UFUNCTION()
	void OnMigrationSessionCreated(bool bSuccess);

	/** 迁移搜索 Session 完成回调 */
	UFUNCTION()
	void OnMigrationSessionsFound(const TArray<FRoomInfo>& Rooms);

	/** 迁移轮询定时器 */
	FTimerHandle MigrationPollTimer;

	/** 迁移轮询计数 */
	int32 MigrationPollCount = 0;

	/** 最大迁移轮询次数（30秒） */
	static constexpr int32 MaxMigrationPolls = 15;

	/** 迁移用 Travel URL */
	FString MigrationTravelURL;

	/** 迁移回调句柄 */
	FDelegateHandle MigrationCreateHandle;
	FDelegateHandle MigrationFindHandle;
};
