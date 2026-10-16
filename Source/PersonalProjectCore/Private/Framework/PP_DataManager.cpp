// Chen Tianbao uses GAS to create online battle games

#include "Framework/PP_DataManager.h"

#include "Engine/DataTable.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

void UPP_DataManager::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	// Config 属性由引擎在 CDO 加载时自动从 DefaultGame.ini 读取，此处仅打印状态便于排查配置缺失
	UE_LOG(LogTemp, Log, TEXT("[DataManager] Subsystem Initialized - Item=%s, Quest=%s, Character=%s"),
		*ItemDataTable.ToString(), *QuestDataTable.ToString(), *CharacterDataTable.ToString());
}

void UPP_DataManager::Deinitialize()
{
	// 释放缓存引用（表资产本身由 GC 管理）
	LoadedItemTable = nullptr;
	LoadedQuestTable = nullptr;
	LoadedCharacterTable = nullptr;
	Super::Deinitialize();
}

UPP_DataManager* UPP_DataManager::Get(const UWorld* World)
{
	if (!World) return nullptr;
	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UPP_DataManager>() : nullptr;
}

// ===== 懒加载 =====

UDataTable* UPP_DataManager::GetTable(TSoftObjectPtr<UDataTable>& SoftRef, TObjectPtr<UDataTable>& Cache, const TCHAR* DebugName)
{
	if (Cache)
	{
		return Cache;
	}
	if (SoftRef.IsNull())
	{
		UE_LOG(LogTemp, Error, TEXT("[DataManager] %s 未配置数据表资产路径，请检查 Config/DefaultGame.ini 的 [/Script/PersonalProjectCore.PP_DataManager] 段"), DebugName);
		return nullptr;
	}
	Cache = SoftRef.LoadSynchronous();
	if (!Cache)
	{
		UE_LOG(LogTemp, Error, TEXT("[DataManager] %s 数据表加载失败: %s"), DebugName, *SoftRef.ToString());
	}
	return Cache;
}

// ===== 查询 API =====

const FPP_ItemInfo* UPP_DataManager::GetItemInfo(FName ItemID)
{
	if (ItemID.IsNone())
	{
		return nullptr;
	}
	UDataTable* Table = GetTable(ItemDataTable, LoadedItemTable, TEXT("ItemDataTable"));
	if (!Table)
	{
		return nullptr;
	}
	static const FString ContextString(TEXT("DataManager.GetItemInfo"));
	return Table->FindRow<FPP_ItemInfo>(ItemID, ContextString, false);
}

const FPP_QuestInfo* UPP_DataManager::GetQuestInfo(FName QuestID)
{
	UDataTable* Table = GetTable(QuestDataTable, LoadedQuestTable, TEXT("QuestDataTable"));
	if (!Table)
	{
		return nullptr;
	}
	static const FString ContextString(TEXT("DataManager.GetQuestInfo"));
	return Table->FindRow<FPP_QuestInfo>(QuestID, ContextString);
}

const FCharacterDataRow* UPP_DataManager::GetCharacterData(FName CharacterId)
{
	if (CharacterId.IsNone())
	{
		return nullptr;
	}
	UDataTable* Table = GetTable(CharacterDataTable, LoadedCharacterTable, TEXT("CharacterDataTable"));
	if (!Table)
	{
		return nullptr;
	}
	static const FString ContextString(TEXT("DataManager.GetCharacterData"));
	return Table->FindRow<FCharacterDataRow>(CharacterId, ContextString);
}

TArray<FName> UPP_DataManager::GetCharacterRowNames()
{
	TArray<FName> RowNames;
	UDataTable* Table = GetTable(CharacterDataTable, LoadedCharacterTable, TEXT("CharacterDataTable"));
	if (Table)
	{
		RowNames = Table->GetRowNames();
	}
	return RowNames;
}
