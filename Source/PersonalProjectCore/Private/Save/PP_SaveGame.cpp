#include "Save/PP_SaveGame.h"

#include "Framework/PP_Log.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

namespace
{
	/** 单项属性是否可用于恢复：必须是有限值且非负 */
	bool IsValidAttributeValue(float Value)
	{
		return FMath::IsFinite(Value) && Value >= 0.f;
	}
}

UPP_SaveGame* UPP_SaveGame::Load(FString& OutError)
{
	OutError.Reset();

	const FString Slot = GetSlotName();

	// 主档缺失但有备份：先恢复，再按正常流程读
	if (!UGameplayStatics::DoesSaveGameExist(Slot, 0))
	{
		if (RestoreBackup())
		{
			UE_LOG(LogPP, Warning, TEXT("[PP_Save] 主存档缺失，已由备份恢复后读取: %s"), *GetBackupFilePath());
		}
		else
		{
			// 无存档文件属于正常情况，不写 Error
			return nullptr;
		}
	}

	UPP_SaveGame* Data = Cast<UPP_SaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));

	// 主档解析失败：尝试用备份覆盖后重读一次
	if (!Data && RestoreBackup())
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 主存档损坏，已用备份恢复: %s"), *GetBackupFilePath());
		Data = Cast<UPP_SaveGame>(UGameplayStatics::LoadGameFromSlot(Slot, 0));
	}

	if (!Data)
	{
		OutError = TEXT("存档文件无法解析");
		UE_LOG(LogPP, Error, TEXT("[PP_Save] 读取失败：%s（槽位=%s）"), *OutError, *Slot);
		return nullptr;
	}

	if (!Data->MigrateToCurrent(OutError))
	{
		UE_LOG(LogPP, Error, TEXT("[PP_Save] 版本迁移失败：%s（存档版本=%d 当前版本=%d）"),
			*OutError, Data->SaveVersion, PPSave::CurrentVersion);
		return nullptr;
	}

	if (!Data->Validate(OutError))
	{
		UE_LOG(LogPP, Error, TEXT("[PP_Save] 存档校验失败：%s（槽位=%s）"), *OutError, *Slot);
		return nullptr;
	}

	return Data;
}

bool UPP_SaveGame::Save(UPP_SaveGame* SaveData, FString& OutError)
{
	OutError.Reset();

	if (!SaveData)
	{
		OutError = TEXT("存档数据为空");
		return false;
	}

	SaveData->SaveVersion = PPSave::CurrentVersion;
	SaveData->SavedAt = FDateTime::Now();

	BackupExisting();

	if (!UGameplayStatics::SaveGameToSlot(SaveData, GetSlotName(), 0))
	{
		OutError = TEXT("写盘失败");
		UE_LOG(LogPP, Error, TEXT("[PP_Save] 保存失败：%s（槽位=%s）"), *OutError, *GetSlotName());
		return false;
	}

	UE_LOG(LogPP, Log, TEXT("[PP_Save] 保存成功：槽位=%s 地图=%s 角色=%s 版本=%d"),
		*GetSlotName(), *SaveData->MapName.ToString(), *SaveData->Character.CharacterId.ToString(), SaveData->SaveVersion);
	return true;
}

bool UPP_SaveGame::HasSave()
{
	return UGameplayStatics::DoesSaveGameExist(GetSlotName(), 0) || IFileManager::Get().FileExists(*GetBackupFilePath());
}

bool UPP_SaveGame::Validate(FString& OutError) const
{
	if (SaveVersion <= 0 || SaveVersion > PPSave::CurrentVersion)
	{
		OutError = FString::Printf(TEXT("存档版本 %d 不受支持"), SaveVersion);
		return false;
	}

	const FPP_SaveAttributeData& A = Character.BaseAttributes;
	const TPair<const TCHAR*, float> Attributes[] = {
		{ TEXT("Health"), A.Health },
		{ TEXT("MaxHealth"), A.MaxHealth },
		{ TEXT("Mana"), A.Mana },
		{ TEXT("MaxMana"), A.MaxMana },
		{ TEXT("Stamina"), A.Stamina },
		{ TEXT("MaxStamina"), A.MaxStamina },
		{ TEXT("Attack"), A.Attack },
		{ TEXT("Defense"), A.Defense },
		{ TEXT("StaminaRegenRate"), A.StaminaRegenRate }
	};

	for (const TPair<const TCHAR*, float>& Pair : Attributes)
	{
		if (!IsValidAttributeValue(Pair.Value))
		{
			OutError = FString::Printf(TEXT("属性 %s 数值非法（%f）"), Pair.Key, Pair.Value);
			return false;
		}
	}

	// 防御性上限：损坏文件可能带出超大数组，逐条跳过发生在组件侧，这里只拦明显异常
	if (Inventory.Items.Num() > 200 || Inventory.Equipped.Num() > 16)
	{
		OutError = FString::Printf(TEXT("背包条目数异常（物品=%d 装备=%d）"),
			Inventory.Items.Num(), Inventory.Equipped.Num());
		return false;
	}

	if (Inventory.Gold < 0)
	{
		OutError = FString::Printf(TEXT("金币数量非法（%d）"), Inventory.Gold);
		return false;
	}

	return true;
}

bool UPP_SaveGame::MigrateToCurrent(FString& OutError)
{
	if (SaveVersion == PPSave::CurrentVersion)
	{
		return true;
	}

	if (SaveVersion > PPSave::CurrentVersion)
	{
		OutError = TEXT("存档来自更新版本的游戏，无法读取");
		return false;
	}

	// 逐级迁移：每新增一版加一步 Migrate_Vn_To_Vm，迁移完成后 SaveVersion 递增。
	// v1 是初始版本，暂无迁移步骤。新增字段只要给了默认值，旧档读出来就是默认值，无需迁移。
	OutError = FString::Printf(TEXT("缺少从版本 %d 到 %d 的迁移步骤"), SaveVersion, PPSave::CurrentVersion);
	return false;
}

void UPP_SaveGame::BackupExisting()
{
	const FString Source = GetSaveFilePath();
	if (!IFileManager::Get().FileExists(*Source))
	{
		return;
	}

	if (IFileManager::Get().Copy(*GetBackupFilePath(), *Source, true, true) == COPY_OK)
	{
		UE_LOG(LogPP, Verbose, TEXT("[PP_Save] 已备份旧存档: %s"), *GetBackupFilePath());
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 备份旧存档失败: %s"), *GetBackupFilePath());
	}
}

bool UPP_SaveGame::RestoreBackup()
{
	const FString Backup = GetBackupFilePath();
	if (!IFileManager::Get().FileExists(*Backup))
	{
		return false;
	}

	return IFileManager::Get().Copy(*GetSaveFilePath(), *Backup, true, true) == COPY_OK;
}

FString UPP_SaveGame::GetSaveFilePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), GetSlotName() + TEXT(".sav"));
}

FString UPP_SaveGame::GetBackupFilePath()
{
	return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("SaveGames"), GetSlotName() + TEXT(".bak"));
}
