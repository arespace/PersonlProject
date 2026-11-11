// P2 安全网 — 数据表配置校验器（编辑器侧，显式触发，不参与运行时循环）

#pragma once

#include "CoreMinimal.h"

#if WITH_EDITOR

class UDataTable;

/** 单张表的校验结果汇总 */
struct FPP_DataValidationReport
{
	int32 RowCount = 0;
	int32 ErrorCount = 0;
	int32 WarningCount = 0;
	TArray<FString> Errors;
	TArray<FString> Warnings;

	void AddError(const FString& Message);
	void AddWarning(const FString& Message);

	/** 按 Error/Warning 级别逐条输出到 LogPP（错误必须能被日志精确定位） */
	void Log(const FString& TableLabel) const;
};

/**
 * 数据表配置校验器
 *
 * 触发方式（只读，不修改任何数据，不注册 Tick/Timer）：
 *   - `pp.ValidateData`          按 Config 路径校验 DT_Items 与遗留 DT_WeaponData
 *   - `pp.ValidateData.SelfTest` 用内存中故意写错的行验证「报错可见」链路（不读写任何资产）
 *
 * 校验项（DT_Items）：
 *   1. RowName 与 ItemID 一致；
 *   2. ItemID 非空且全表唯一；
 *   3. 武器行（配置了 WeaponConfig 的行）EquipSlot 必须是 MainHand / OffHand；
 *   4. EquipEffectClass 非空时 ItemType 必须是 Equipment；
 *   5. 非 Equipment 行不得配置 EquipSlot。
 *
 * 校验项（DT_Enemies）：
 *   1. RowName 与 EnemyID 一致且非空、全表唯一；
 *   2. DropItemID 非空时必须存在于 DT_Items；
 *   3. 攻击距离/延迟/冷却/掉落数量等数值范围合法；
 *   4. Boss 阶段阈值必须落在 0~1。
 *
 * 校验项（DT_Gatherables）：
 *   1. RowName 与 GatherableID 一致且非空、全表唯一；
 *   2. ItemID 非空时必须存在于 DT_Items；
 *   3. 采集数量/耗时/次数/重生时间/交互半径等数值范围合法（0 表示沿用蓝图现值）。
 *
 * 校验项（DT_Characters）：
 *   1. 行结构必须是 FCharacterDataRow；
 *   2. 成长/手感字段不能为负（0 表示沿用组件/代码现值）；
 *   3. 锁定搜索角不得超过 180 度；解除距离需大于搜索半径（否则没有滞回）。
 *
 * 说明：DT_WeaponData 已在武器装备改造中并入 DT_Items.WeaponConfig，
 *      现行工程内不应存在该资产；校验器只在资产仍存在时给出提示，不存在则跳过。
 */
class PERSONALPROJECTCORE_API FPP_DataValidator
{
public:
	/** 校验物品表；TableLabel 仅用于日志标识，为空时取表资产名 */
	static FPP_DataValidationReport ValidateItemTable(UDataTable* ItemTable, const FString& TableLabel = FString());

	/** 检查遗留武器表是否仍存在（已并入 DT_Items，存在即提示） */
	static FPP_DataValidationReport ValidateLegacyWeaponTable();

	/** 校验敌人表；ItemTable 用于校验 DropItemID 外键，为空时按 Config 路径懒加载 */
	static FPP_DataValidationReport ValidateEnemyTable(UDataTable* EnemyTable, UDataTable* ItemTable = nullptr,
		const FString& TableLabel = FString());

	/** 校验采集点表；ItemTable 用于校验 ItemID 外键 */
	static FPP_DataValidationReport ValidateGatherableTable(UDataTable* GatherableTable, UDataTable* ItemTable = nullptr,
		const FString& TableLabel = FString());

	/** 校验角色表（行结构与成长/手感数值范围） */
	static FPP_DataValidationReport ValidateCharacterTable(UDataTable* CharacterTable, const FString& TableLabel = FString());

	/** 按 Config/DefaultGame.ini 的表路径校验全部目标表，返回错误总数 */
	static int32 ValidateAll();

	/** 自检：用内存中故意写错的行跑一遍校验，返回检出的错误数（不读写任何资产） */
	static int32 RunSelfTest();
};

#endif // WITH_EDITOR
