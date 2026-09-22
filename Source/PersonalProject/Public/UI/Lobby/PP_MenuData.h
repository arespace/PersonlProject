// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PP_MenuData.generated.h"

/**
 * 登录菜单数据行 - 定义登录界面的菜单项配置
 * 作为 DataTable (DT_LoginMenu) 的行结构使用，菜单项由表格配置
 */
USTRUCT(BlueprintType)
struct FLoginMenuItemRow : public FTableRowBase
{
	GENERATED_BODY()

	/** 菜单显示名 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Login")
	FString Name;

	/** 图标软引用路径 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Login")
	FString IconPath;

	/** 点击行为标识：NewGame / Continue / Settings / Quit / ReturnToMain */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Login")
	FName Action;
};

/**
 * 菜单数据读取库 - 从 DT_LoginMenu 读取菜单项并回传给 Lua
 * 数据（DataTable）与逻辑（Lua）分离
 */
UCLASS()
class PERSONALPROJECT_API UPP_MenuDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 读取 DT_LoginMenu 的全部行，返回给 Lua 遍历 */
	UFUNCTION(BlueprintCallable, Category = "PP|Login")
	static TArray<FLoginMenuItemRow> GetLoginMenuItems();
};
