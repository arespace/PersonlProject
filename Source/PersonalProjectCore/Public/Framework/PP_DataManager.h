// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Characters/PP_CharacterData.h"
#include "Data/PP_ItemData.h"
#include "Data/PP_QuestData.h"
#include "PP_DataManager.generated.h"

class UDataTable;

/**
 * UPP_DataManager - 全局 DataTable 统一管理器 (GameInstanceSubsystem)
 *
 * 统一持有项目 3 张全局数据表（物品 / 任务 / 角色），
 * 各组件不再各自持有 UDataTable，一律通过本子系统查询。
 *
 * !!! 新代码禁止直接持有 DataTable，一律走 DataManager !!!
 * 例外（实例级数据保留在实例上，不经此管理）：
 *   - NPC 的 DialogueTable（每 NPC 对话不同）
 *   - ShopComponent 的 ShopDataTable（每商人商品/库存不同）
 *
 * 表资产路径通过 Config=Game 配置（Config/DefaultGame.ini 的
 * [/Script/PersonalProject.PP_DataManager] 段），首次访问时懒加载并缓存。
 */
UCLASS(Config = Game)
class PERSONALPROJECTCORE_API UPP_DataManager : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	//~ Begin UGameInstanceSubsystem Interface
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~ End UGameInstanceSubsystem Interface

	/** 便捷静态方法：从 World 获取 DataManager 实例 */
	static UPP_DataManager* Get(const UWorld* World);

	// ===== 查询 API =====

	/** 按 ItemID（= 行名）查找物品配置，行缺失不告警（沿用原 Inventory 查询行为） */
	const FPP_ItemInfo* GetItemInfo(FName ItemID);

	/** 按 QuestID（= 行名）查找任务配置，行缺失告警（沿用原 Quest 查询行为） */
	const FPP_QuestInfo* GetQuestInfo(FName QuestID);

	/** 按 CharacterId（= 行名）查找角色外观配置 */
	const FCharacterDataRow* GetCharacterData(FName CharacterId);

	/** 获取角色表全部行名（角色选择界面动态填充角色列表用） */
	TArray<FName> GetCharacterRowNames();

	/** 角色表是否可用（已配置且加载成功），用于调用方在产生副作用前校验 */
	bool HasCharacterTable()
	{
		return GetTable(CharacterDataTable, LoadedCharacterTable, TEXT("CharacterDataTable")) != nullptr;
	}

private:
	/**
	 * 懒加载：缓存有效直接返回；否则 LoadSynchronous 加载并缓存，
	 * 未配置或加载失败打 UE_LOG Error 并返回 nullptr
	 */
	UDataTable* GetTable(TSoftObjectPtr<UDataTable>& SoftRef, TObjectPtr<UDataTable>& Cache, const TCHAR* DebugName);

	// ===== 配置（Config/DefaultGame.ini） =====

	/** 物品数据表资产路径（FPP_ItemInfo 行结构） */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> ItemDataTable;

	/** 任务数据表资产路径（FPP_QuestInfo 行结构） */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> QuestDataTable;

	/** 角色数据表资产路径（FCharacterDataRow 行结构） */
	UPROPERTY(Config, EditAnywhere, Category = "Data")
	TSoftObjectPtr<UDataTable> CharacterDataTable;

	// ===== 已加载表缓存 =====

	UPROPERTY()
	TObjectPtr<UDataTable> LoadedItemTable;

	UPROPERTY()
	TObjectPtr<UDataTable> LoadedQuestTable;

	UPROPERTY()
	TObjectPtr<UDataTable> LoadedCharacterTable;
};
