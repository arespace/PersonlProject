// 存档系统 — 存档数据结构
// 只放纯数据，不放逻辑；所有字段必须带 UPROPERTY，才能被 USaveGame 序列化。
// 版本升级时新增字段一律给默认值，旧档缺失字段读出来即默认值。

#pragma once

#include "CoreMinimal.h"
#include "PP_SaveTypes.generated.h"

/**
 * 角色进度存档。
 * 属性只存 base 值：装备 GE 的 Additive Modifier 不落盘，读档时由装备恢复流程重新施加，
 * 避免"先写属性再恢复装备"导致加成叠两次。
 */
USTRUCT()
struct FPP_SaveAttributeData
{
	GENERATED_BODY()

	UPROPERTY() float Health = 0.f;
	UPROPERTY() float MaxHealth = 0.f;
	UPROPERTY() float Mana = 0.f;
	UPROPERTY() float MaxMana = 0.f;
	UPROPERTY() float Stamina = 0.f;
	UPROPERTY() float MaxStamina = 0.f;
	UPROPERTY() float Attack = 0.f;
	UPROPERTY() float Defense = 0.f;
	UPROPERTY() float StaminaRegenRate = 0.f;
};

USTRUCT()
struct FPP_SaveCharacterData
{
	GENERATED_BODY()

	/** 角色 ID（DT_Characters / GameMode 能力映射的行名） */
	UPROPERTY() FName CharacterId;

	/** 九项属性的 base 值 */
	UPROPERTY() FPP_SaveAttributeData BaseAttributes;
};
