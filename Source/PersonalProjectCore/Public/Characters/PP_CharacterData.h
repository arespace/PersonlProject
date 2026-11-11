// Copyright Personal Project. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PP_CharacterData.generated.h"

/**
 * 角色数据行 - 定义可选角色的配置信息
 * 作为 DataTable (DT_Characters) 的行结构使用
 */
USTRUCT(BlueprintType)
struct FCharacterDataRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 角色显示名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character")
	FText DisplayName;

	/** 骨骼网格体引用（角色模型） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character")
	TSoftObjectPtr<USkeletalMesh> CharacterMesh;

	/** 动画蓝图类引用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character")
	TSoftClassPtr<UAnimInstance> AnimBlueprintClass;

	/** 角色图标（用于选择按钮 UI） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character")
	TSoftObjectPtr<UTexture2D> CharacterIcon;

	/** 角色描述文本 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character")
	FText Description;

	// ===== 成长 / 手感 =====
	// 默认值即代码/组件现值；表内填 0 表示未配置，沿用现值（行为不变）。

	/** 翻滚蒙太奇播放速率（倍率）；<= 0 沿用代码默认 1.3 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0"))
	float RollPlayRate{1.3f};

	/** 翻滚体力消耗；<= 0 沿用耐力组件现值 20 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0"))
	float RollStaminaCost{20.f};

	/** 锁定搜索半径（cm）；<= 0 沿用锁定组件现值 1200 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0"))
	float LockOnSearchRadius{1200.f};

	/** 锁定解除距离（cm，需大于搜索半径形成滞回）；<= 0 沿用锁定组件现值 1600 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0"))
	float LockOnBreakDistance{1600.f};

	/** 锁定目标的相机前方半角（度）；<= 0 沿用锁定组件现值 60 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0", ClampMax = "180.0"))
	float LockOnMaxScreenAngle{60.f};

	/** 相机朝向目标的插值速度；<= 0 沿用锁定组件现值 10 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0"))
	float LockOnCameraInterpolationSpeed{10.f};

	/** 锁定焦点相对敌人原点的高度偏移（cm）；<= 0 沿用锁定组件现值 60 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Character|Growth", Meta = (ClampMin = "0.0"))
	float LockOnFocusHeight{60.f};
};
