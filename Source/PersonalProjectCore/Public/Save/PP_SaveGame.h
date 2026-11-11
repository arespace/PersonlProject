// 存档系统 — 存档载体
// USaveGame 子类 + 磁盘读写 / 备份 / 版本迁移。
// 单槽（Slot0）；偏好类存档 UPP_LocalSaveGame 保持独立，互不覆盖。

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Save/PP_SaveTypes.h"
#include "PP_SaveGame.generated.h"

/** 当前存档结构版本；只在新结构无法从旧档默认值推导时才 +1 */
namespace PPSave
{
	constexpr int32 CurrentVersion = 1;
}

/**
 * UPP_SaveGame - 游戏进度存档
 *
 * 用法（服务端）：
 *   FString Error;
 *   UPP_SaveGame* Data = UPP_SaveGame::Load(Error);   // 无档/损坏返回 nullptr，Error 说明原因
 *   UPP_SaveGame::Save(Data, Error);                  // 覆盖前自动备份 .bak
 */
UCLASS()
class PERSONALPROJECTCORE_API UPP_SaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 存档结构版本（不等于工程版本） */
	UPROPERTY() int32 SaveVersion = 1;

	UPROPERTY() FDateTime SavedAt;

	/** 存档所在关卡（读档时用于校验/回到原地图） */
	UPROPERTY() FName MapName;

	/** 存档玩家名（仅用于展示与多人时的区分） */
	UPROPERTY() FString PlayerName;

	/** 槽位号；v1 固定 0，预留多槽 */
	UPROPERTY() int32 SlotIndex = 0;

	UPROPERTY() FPP_SaveCharacterData Character;

	// ===== 读写入口（服务端调用） =====

	/** 读取存档；无档返回 nullptr 且 Error 为空；损坏时尝试 .bak，仍失败时 Error 给出原因 */
	static UPP_SaveGame* Load(FString& OutError);

	/** 写回磁盘；覆盖前把现有 .sav 备份为 .bak */
	static bool Save(UPP_SaveGame* SaveData, FString& OutError);

	/** 是否存在存档文件 */
	static bool HasSave();

	/** 槽位名（文件名 = <SlotName>.sav） */
	static FString GetSlotName() { return TEXT("PP_Save_Slot0"); }

private:
	/** 版本校验 + 数值合法性校验 */
	bool Validate(FString& OutError) const;

	/** 旧版本升到当前版本；v1 无迁移步骤 */
	bool MigrateToCurrent(FString& OutError);

	/** 把已存在的 .sav 复制为 .bak */
	static void BackupExisting();

	/** 用 .bak 覆盖损坏的 .sav；成功返回 true */
	static bool RestoreBackup();

	/** .sav / .bak 的绝对路径 */
	static FString GetSaveFilePath();
	static FString GetBackupFilePath();
};
