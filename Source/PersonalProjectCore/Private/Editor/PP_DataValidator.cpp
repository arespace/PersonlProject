// P2 安全网 — 数据表配置校验器实现（只读；显式触发，不参与运行时循环）

#include "Editor/PP_DataValidator.h"

#if WITH_EDITOR

#include "Engine/DataTable.h"
#include "GameplayEffect.h"
#include "Abilities/GameplayAbility.h"
#include "HAL/IConsoleManager.h"
#include "Misc/ConfigCacheIni.h"
#include "Misc/PackageName.h"
#include "UObject/Package.h"

#include "Data/PP_EnemyData.h"
#include "Data/PP_ItemData.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_Log.h"

namespace
{
	/** 遗留武器表路径：装备改造后武器配置已并入 DT_Items.WeaponConfig，正常工程内不应存在 */
	const TCHAR* GLegacyWeaponTablePath = TEXT("/Game/PersonalProject/Data/DT_WeaponData.DT_WeaponData");

	/** 从 DefaultGame.ini 的 DataManager 段读取表资产路径（与运行时读取同一份配置，避免两处口径） */
	FString ReadDataManagerTablePath(const TCHAR* Key)
	{
		if (!GConfig)
		{
			return FString();
		}

		FString Path;
		const FString Section = UPP_DataManager::StaticClass()->GetPathName();
		GConfig->GetString(*Section, Key, Path, GGameIni);
		return Path;
	}

	template <typename TEnum>
	FString EnumToName(TEnum Value)
	{
		if (const UEnum* Enum = StaticEnum<TEnum>())
		{
			return Enum->GetNameStringByValue(static_cast<int64>(Value));
		}
		return FString::Printf(TEXT("%d"), static_cast<int32>(Value));
	}
}

// ===== FPP_DataValidationReport =====

void FPP_DataValidationReport::AddError(const FString& Message)
{
	++ErrorCount;
	Errors.Add(Message);
}

void FPP_DataValidationReport::AddWarning(const FString& Message)
{
	++WarningCount;
	Warnings.Add(Message);
}

void FPP_DataValidationReport::Log(const FString& TableLabel) const
{
	for (const FString& Message : Errors)
	{
		UE_LOG(LogPP, Error, TEXT("[DataValidator] %s：%s"), *TableLabel, *Message);
	}
	for (const FString& Message : Warnings)
	{
		UE_LOG(LogPP, Warning, TEXT("[DataValidator] %s：%s"), *TableLabel, *Message);
	}
	UE_LOG(LogPP, Log, TEXT("[DataValidator] %s 校验完成：行数=%d 错误=%d 警告=%d"),
		*TableLabel, RowCount, ErrorCount, WarningCount);
}

// ===== FPP_DataValidator =====

FPP_DataValidationReport FPP_DataValidator::ValidateItemTable(UDataTable* ItemTable, const FString& TableLabel)
{
	FPP_DataValidationReport Report;
	const FString Label = !TableLabel.IsEmpty()
		? TableLabel
		: (ItemTable ? ItemTable->GetName() : FString(TEXT("<未加载>")));

	if (!ItemTable)
	{
		Report.AddError(TEXT("数据表未加载（配置路径为空或资产不存在），无法校验"));
		Report.Log(Label);
		return Report;
	}

	if (ItemTable->GetRowStruct() != FPP_ItemInfo::StaticStruct())
	{
		Report.AddError(FString::Printf(TEXT("行结构不是 FPP_ItemInfo（当前：%s），无法按物品表校验"),
			ItemTable->GetRowStruct() ? *ItemTable->GetRowStruct()->GetName() : TEXT("None")));
		Report.Log(Label);
		return Report;
	}

	const TArray<FName> RowNames = ItemTable->GetRowNames();
	Report.RowCount = RowNames.Num();
	if (RowNames.IsEmpty())
	{
		Report.AddWarning(TEXT("表内没有任何行"));
	}

	// ItemID -> 首次出现的行名，用于重复检测
	TMap<FName, FName> ItemIDOwner;

	for (const FName& RowName : RowNames)
	{
		const FPP_ItemInfo* Row = ItemTable->FindRow<FPP_ItemInfo>(RowName, TEXT("PP_DataValidator"), false);
		if (!Row)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s' 无法按 FPP_ItemInfo 解析（行数据损坏或行结构不匹配）"), *RowName.ToString()));
			continue;
		}

		// 1. ItemID 非空
		if (Row->ItemID.IsNone())
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：ItemID 为空（必须与 RowName 一致且非空）"), *RowName.ToString()));
		}
		// 2. RowName 与 ItemID 一致
		else
		{
			if (Row->ItemID != RowName)
			{
				Report.AddError(FString::Printf(TEXT("行 '%s'：RowName 与 ItemID 不一致（RowName='%s'，ItemID='%s'）"),
					*RowName.ToString(), *RowName.ToString(), *Row->ItemID.ToString()));
			}

			// 3. ItemID 全表唯一
			if (const FName* FirstRow = ItemIDOwner.Find(Row->ItemID))
			{
				Report.AddError(FString::Printf(TEXT("行 '%s'：ItemID '%s' 重复（已在行 '%s' 使用）"),
					*RowName.ToString(), *Row->ItemID.ToString(), *FirstRow->ToString()));
			}
			else
			{
				ItemIDOwner.Add(Row->ItemID, RowName);
			}
		}

		const FPP_WeaponConfig& WeaponConfig = Row->WeaponConfig;
		const bool bHasWeaponConfig =
			WeaponConfig.WeaponAbilityClass != nullptr
			|| !WeaponConfig.Visual.Mesh.IsNull()
			|| !WeaponConfig.Visual.AttachSocket.IsNone()
			|| WeaponConfig.bCanUseOffHand;

		const bool bIsWeaponSlot =
			Row->EquipSlot == EPP_EquipSlot::MainHand || Row->EquipSlot == EPP_EquipSlot::OffHand;

		// 4. 武器行只能落在主手/副手
		if (bHasWeaponConfig && !bIsWeaponSlot)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：配置了武器配置（WeaponConfig），EquipSlot 必须是 MainHand 或 OffHand（当前：%s）"),
				*RowName.ToString(), *EnumToName(Row->EquipSlot)));
		}

		if (Row->ItemType == EPP_ItemType::Equipment && bIsWeaponSlot && !bHasWeaponConfig)
		{
			Report.AddWarning(FString::Printf(TEXT("行 '%s'：主手/副手装备未配置 WeaponConfig（无外观与专属技能）"), *RowName.ToString()));
		}

		// 5. EquipEffectClass 存在时 ItemType 必须合理（穿戴效果只对 Equipment 生效）
		if (Row->EquipEffectClass != nullptr && Row->ItemType != EPP_ItemType::Equipment)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：配置了 EquipEffectClass '%s'，但 ItemType 是 %s（穿戴效果只对 Equipment 生效）"),
				*RowName.ToString(), *Row->EquipEffectClass->GetName(), *EnumToName(Row->ItemType)));
		}

		// 6. 非 Equipment 行不得配置槽位
		if (Row->ItemType != EPP_ItemType::Equipment && Row->EquipSlot != EPP_EquipSlot::None)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：ItemType 是 %s，却配置了 EquipSlot=%s"),
				*RowName.ToString(), *EnumToName(Row->ItemType), *EnumToName(Row->EquipSlot)));
		}
	}

	Report.Log(Label);
	return Report;
}

FPP_DataValidationReport FPP_DataValidator::ValidateEnemyTable(UDataTable* EnemyTable, UDataTable* ItemTable, const FString& TableLabel)
{
	FPP_DataValidationReport Report;
	const FString Label = !TableLabel.IsEmpty()
		? TableLabel
		: (EnemyTable ? EnemyTable->GetName() : FString(TEXT("<未加载>")));

	if (!EnemyTable)
	{
		Report.AddError(TEXT("数据表未加载（配置路径为空或资产不存在），无法校验"));
		Report.Log(Label);
		return Report;
	}

	if (EnemyTable->GetRowStruct() != FPP_EnemyInfo::StaticStruct())
	{
		Report.AddError(FString::Printf(TEXT("行结构不是 FPP_EnemyInfo（当前：%s），无法按敌人表校验"),
			EnemyTable->GetRowStruct() ? *EnemyTable->GetRowStruct()->GetName() : TEXT("None")));
		Report.Log(Label);
		return Report;
	}

	const TArray<FName> RowNames = EnemyTable->GetRowNames();
	Report.RowCount = RowNames.Num();
	if (RowNames.IsEmpty())
	{
		Report.AddWarning(TEXT("表内没有任何行：所有敌人仍按蓝图上的原值运行"));
	}

	TMap<FName, FName> EnemyIDOwner;

	for (const FName& RowName : RowNames)
	{
		const FPP_EnemyInfo* Row = EnemyTable->FindRow<FPP_EnemyInfo>(RowName, TEXT("PP_DataValidator"), false);
		if (!Row)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s' 无法按 FPP_EnemyInfo 解析（行数据损坏或行结构不匹配）"), *RowName.ToString()));
			continue;
		}

		// 1. EnemyID 非空且与行名一致
		if (Row->EnemyID.IsNone())
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：EnemyID 为空（必须与 RowName 一致且非空）"), *RowName.ToString()));
		}
		else
		{
			if (Row->EnemyID != RowName)
			{
				Report.AddError(FString::Printf(TEXT("行 '%s'：RowName 与 EnemyID 不一致（RowName='%s'，EnemyID='%s'）"),
					*RowName.ToString(), *RowName.ToString(), *Row->EnemyID.ToString()));
			}

			// 2. EnemyID 全表唯一
			if (const FName* FirstRow = EnemyIDOwner.Find(Row->EnemyID))
			{
				Report.AddError(FString::Printf(TEXT("行 '%s'：EnemyID '%s' 重复（已在行 '%s' 使用）"),
					*RowName.ToString(), *Row->EnemyID.ToString(), *FirstRow->ToString()));
			}
			else
			{
				EnemyIDOwner.Add(Row->EnemyID, RowName);
			}
		}

		// 3. 数值范围
		if (Row->AcceptanceRadius <= 0.f)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：AcceptanceRadius 必须大于 0（当前 %.1f）"),
				*RowName.ToString(), Row->AcceptanceRadius));
		}
		if (Row->MinAttackDelay > Row->MaxAttackDelay)
		{
			Report.AddWarning(FString::Printf(TEXT("行 '%s'：MinAttackDelay(%.2f) 大于 MaxAttackDelay(%.2f)，随机区间为空"),
				*RowName.ToString(), Row->MinAttackDelay, Row->MaxAttackDelay));
		}
		if (Row->AttackCooldownMin > Row->AttackCooldownMax)
		{
			Report.AddWarning(FString::Printf(TEXT("行 '%s'：AttackCooldownMin(%.2f) 大于 AttackCooldownMax(%.2f)，随机区间为空"),
				*RowName.ToString(), Row->AttackCooldownMin, Row->AttackCooldownMax));
		}
		if (Row->GoldDropAmount < 0 || Row->DropItemCount < 0)
		{
			Report.AddError(FString::Printf(TEXT("行 '%s'：GoldDropAmount(%d) / DropItemCount(%d) 不能为负"),
				*RowName.ToString(), Row->GoldDropAmount, Row->DropItemCount));
		}

		// 4. DropItemID 外键必须命中 DT_Items
		if (!Row->DropItemID.IsNone())
		{
			if (Row->DropItemCount <= 0)
			{
				Report.AddWarning(FString::Printf(TEXT("行 '%s'：配置了 DropItemID '%s' 但 DropItemCount 为 %d，实际不会发放"),
					*RowName.ToString(), *Row->DropItemID.ToString(), Row->DropItemCount));
			}

			if (!ItemTable)
			{
				Report.AddWarning(FString::Printf(TEXT("行 '%s'：DropItemID '%s' 无法校验（DT_Items 未加载）"),
					*RowName.ToString(), *Row->DropItemID.ToString()));
			}
			else if (!ItemTable->FindRow<FPP_ItemInfo>(Row->DropItemID, TEXT("PP_DataValidator"), false))
			{
				Report.AddError(FString::Printf(TEXT("行 '%s'：DropItemID '%s' 在 DT_Items 中不存在"),
					*RowName.ToString(), *Row->DropItemID.ToString()));
			}
		}

		// 5. Boss 阶段阈值必须在 0~1
		for (const float Threshold : Row->BossPhaseThresholds)
		{
			if (Threshold <= 0.f || Threshold > 1.f)
			{
				Report.AddError(FString::Printf(TEXT("行 '%s'：BossPhaseThresholds 含非法阈值 %.2f（必须在 0~1 之间）"),
					*RowName.ToString(), Threshold));
			}
		}
	}

	Report.Log(Label);
	return Report;
}

FPP_DataValidationReport FPP_DataValidator::ValidateLegacyWeaponTable()
{
	FPP_DataValidationReport Report;
	const FString Path = GLegacyWeaponTablePath;

	if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(Path)))
	{
		UE_LOG(LogPP, Log, TEXT("[DataValidator] 遗留武器表 %s 不存在（武器配置已并入 DT_Items.WeaponConfig），跳过"), *Path);
		return Report;
	}

	UDataTable* LegacyTable = LoadObject<UDataTable>(nullptr, *Path);
	if (!LegacyTable)
	{
		Report.AddError(FString::Printf(TEXT("资产存在但加载失败：%s"), *Path));
		Report.Log(Path);
		return Report;
	}

	Report.RowCount = LegacyTable->GetRowNames().Num();
	Report.AddWarning(FString::Printf(
		TEXT("遗留武器表仍存在（%d 行）：武器外观与技能的唯一来源应为 DT_Items.WeaponConfig，请确认是否还有消费方引用该资产"),
		Report.RowCount));
	Report.Log(Path);
	return Report;
}

int32 FPP_DataValidator::ValidateAll()
{
	UE_LOG(LogPP, Log, TEXT("[DataValidator] ===== 开始校验（只读，不修改任何资产）====="));

	int32 TotalErrors = 0;
	int32 TotalWarnings = 0;

	// DT_Items
	UDataTable* LoadedItemTable = nullptr;
	const FString ItemPath = ReadDataManagerTablePath(TEXT("ItemDataTable"));
	if (ItemPath.IsEmpty())
	{
		UE_LOG(LogPP, Error, TEXT("[DataValidator] DT_Items 路径为空：请检查 Config/DefaultGame.ini 的 [%s] 段 ItemDataTable"),
			*UPP_DataManager::StaticClass()->GetPathName());
		++TotalErrors;
	}
	else
	{
		LoadedItemTable = LoadObject<UDataTable>(nullptr, *ItemPath);
		if (!LoadedItemTable)
		{
			UE_LOG(LogPP, Error, TEXT("[DataValidator] DT_Items 加载失败：%s"), *ItemPath);
			++TotalErrors;
		}
		else
		{
			const FPP_DataValidationReport Report = ValidateItemTable(LoadedItemTable, FString::Printf(TEXT("DT_Items(%s)"), *ItemPath));
			TotalErrors += Report.ErrorCount;
			TotalWarnings += Report.WarningCount;
		}
	}

	// DT_Enemies（可缺省：资产不存在时只提示，不计错误，便于分批落地）
	const FString EnemyPath = ReadDataManagerTablePath(TEXT("EnemyDataTable"));
	if (EnemyPath.IsEmpty())
	{
		UE_LOG(LogPP, Warning, TEXT("[DataValidator] DT_Enemies 路径未配置，跳过敌人表校验"));
	}
	else if (!FPackageName::DoesPackageExist(FPackageName::ObjectPathToPackageName(EnemyPath)))
	{
		UE_LOG(LogPP, Warning, TEXT("[DataValidator] DT_Enemies 资产不存在（%s），敌人仍走蓝图配置，跳过校验"), *EnemyPath);
	}
	else
	{
		UDataTable* EnemyTable = LoadObject<UDataTable>(nullptr, *EnemyPath);
		if (!EnemyTable)
		{
			UE_LOG(LogPP, Error, TEXT("[DataValidator] DT_Enemies 加载失败：%s"), *EnemyPath);
			++TotalErrors;
		}
		else
		{
			const FPP_DataValidationReport Report = ValidateEnemyTable(EnemyTable, LoadedItemTable,
				FString::Printf(TEXT("DT_Enemies(%s)"), *EnemyPath));
			TotalErrors += Report.ErrorCount;
			TotalWarnings += Report.WarningCount;
		}
	}

	// 遗留 DT_WeaponData（已并入 DT_Items）
	{
		const FPP_DataValidationReport Report = ValidateLegacyWeaponTable();
		TotalErrors += Report.ErrorCount;
		TotalWarnings += Report.WarningCount;
	}

	// UE_LOG 的 Verbosity 必须是裸标识符，不能写成三元表达式
	if (TotalErrors > 0)
	{
		UE_LOG(LogPP, Error, TEXT("[DataValidator] ===== 校验结束：错误 %d 个，警告 %d 个 ====="), TotalErrors, TotalWarnings);
	}
	else
	{
		UE_LOG(LogPP, Log, TEXT("[DataValidator] ===== 校验结束：错误 0 个，警告 %d 个 ====="), TotalWarnings);
	}
	return TotalErrors;
}

int32 FPP_DataValidator::RunSelfTest()
{
	UE_LOG(LogPP, Log, TEXT("[DataValidator] ===== 自检：用内存中的错误行验证报错链路（不读写任何资产）====="));

	UDataTable* TransientTable = NewObject<UDataTable>(GetTransientPackage(), NAME_None, RF_Transient);
	TransientTable->RowStruct = FPP_ItemInfo::StaticStruct();

	// 故意写错：RowName 与 ItemID 不一致
	FPP_ItemInfo WrongNameRow;
	WrongNameRow.ItemID = TEXT("Sword_Test");
	TransientTable->AddRow(FName(TEXT("WrongRowName")), WrongNameRow);

	// 故意写错：武器配置挂在防具槽
	FPP_ItemInfo WrongSlotRow;
	WrongSlotRow.ItemID = TEXT("WrongSlot_Test");
	WrongSlotRow.ItemType = EPP_ItemType::Equipment;
	WrongSlotRow.EquipSlot = EPP_EquipSlot::Head;
	WrongSlotRow.WeaponConfig.Visual.AttachSocket = TEXT("weapon_r");
	TransientTable->AddRow(FName(TEXT("WrongSlot_Test")), WrongSlotRow);

	const FPP_DataValidationReport Report = ValidateItemTable(TransientTable, TEXT("<自检内存表>"));
	constexpr int32 ExpectedErrors = 2;

	if (Report.ErrorCount >= ExpectedErrors)
	{
		UE_LOG(LogPP, Log, TEXT("[DataValidator] 自检通过：检出 %d 个错误（预期至少 %d 个）"), Report.ErrorCount, ExpectedErrors);
	}
	else
	{
		UE_LOG(LogPP, Error, TEXT("[DataValidator] 自检失败：只检出 %d 个错误（预期至少 %d 个），报错链路不可用"),
			Report.ErrorCount, ExpectedErrors);
	}
	return Report.ErrorCount;
}

// ===== 控制台入口（编辑器控制台 / PIE 控制台均可执行）=====

static FAutoConsoleCommand GPPValidateDataCommand(
	TEXT("pp.ValidateData"),
	TEXT("校验 DT_Items、DT_Enemies 与遗留 DT_WeaponData 的数据配置（只读，不修改数据）"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		FPP_DataValidator::ValidateAll();
	}));

static FAutoConsoleCommand GPPValidateDataSelfTestCommand(
	TEXT("pp.ValidateData.SelfTest"),
	TEXT("用内存中故意写错的行验证数据校验器能否报出明确错误（不读写资产）"),
	FConsoleCommandDelegate::CreateLambda([]()
	{
		FPP_DataValidator::RunSelfTest();
	}));

#endif // WITH_EDITOR
