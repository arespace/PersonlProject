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

	bool IsFiniteVector(const FVector& Value)
	{
		return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y) && FMath::IsFinite(Value.Z);
	}

	/**
	 * 版本迁移步骤表：下标 = 源版本号，值为「把源版本迁到源版本 + 1」的函数。
	 * 新增版本时只在末尾追加一步，不要改已有步骤：
	 *   CurrentVersion 1 → 2 时，填入 Steps[1] = Migrate_V1_To_V2；
	 *   步骤内部负责补齐新字段，并把 SaveVersion 设为 2。
	 */
	using FPP_SaveMigrationStep = bool (*)(UPP_SaveGame&, FString&);

	const FPP_SaveMigrationStep GMigrationSteps[] = { nullptr }; // 下标 0 不使用
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

	// 没有地图名的存档无法确定"继续游戏"该去哪个关卡
	if (MapName.IsNone())
	{
		OutError = TEXT("存档缺少地图名");
		return false;
	}

	// 检查点只在 bHasCheckpoint 时生效；坐标/缩放必须是有限值，
	// 否则 RestartPlayer 会拿 NaN 做 TeleportTo
	if (Checkpoint.bHasCheckpoint
		&& (!IsFiniteVector(Checkpoint.Transform.GetLocation())
			|| !IsFiniteVector(Checkpoint.Transform.GetScale3D())))
	{
		OutError = TEXT("检查点坐标非法");
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

	if (Quests.ActiveQuests.Num() > 50 || Quests.CompletedQuestIDs.Num() > 200)
	{
		OutError = FString::Printf(TEXT("任务条目数异常（活跃=%d 已完成=%d）"),
			Quests.ActiveQuests.Num(), Quests.CompletedQuestIDs.Num());
		return false;
	}

	if (World.Gatherables.Num() > 500 || World.Bosses.Num() > 20 || World.Souls.Num() > 8)
	{
		OutError = FString::Printf(TEXT("世界状态条目数异常（采集点=%d Boss=%d 魂=%d）"),
			World.Gatherables.Num(), World.Bosses.Num(), World.Souls.Num());
		return false;
	}

	// 单条世界状态数据非法时不在这里整体失败：读取时逐条跳过并告警
	return true;
}

bool UPP_SaveGame::MigrateToCurrent(FString& OutError)
{
	if (SaveVersion <= 0)
	{
		OutError = TEXT("存档版本号非法");
		return false;
	}

	if (SaveVersion > PPSave::CurrentVersion)
	{
		OutError = TEXT("存档来自更新版本的游戏，无法读取");
		return false;
	}

	// 逐级迁移：每一步只负责把版本 +1，并自行补齐该版本新增字段的默认值。
	// 迁移表为空时（v1）循环不会进入；新增版本时在 GMigrationSteps 末尾追加一步即可。
	while (SaveVersion < PPSave::CurrentVersion)
	{
		const int32 FromVersion = SaveVersion;
		if (FromVersion < 0 || FromVersion >= static_cast<int32>(UE_ARRAY_COUNT(GMigrationSteps))
			|| GMigrationSteps[FromVersion] == nullptr)
		{
			OutError = FString::Printf(TEXT("缺少从版本 %d 到 %d 的迁移步骤"), FromVersion, FromVersion + 1);
			return false;
		}

		if (!GMigrationSteps[FromVersion](*this, OutError))
		{
			return false;
		}

		// 迁移步骤必须推进版本号，否则下面的循环会死循环
		if (SaveVersion <= FromVersion)
		{
			OutError = FString::Printf(TEXT("版本 %d 的迁移步骤未推进版本号"), FromVersion);
			return false;
		}
	}

	return true;
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
