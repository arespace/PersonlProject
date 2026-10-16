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
};
