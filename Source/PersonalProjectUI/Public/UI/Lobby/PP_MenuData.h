// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PP_MenuData.generated.h"

class UTexture2D;

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

	/**
	 * 图标资产（软引用）—— 填表时只需要填这一列
	 *
	 * 为什么必须是资产引用而不是 FString 路径：烘焙器沿引用图决定哪些资产进包，
	 * 软引用（TSoftObjectPtr）会被 gather，纯 FString 路径它看不见 ——
	 * 这正是打包后登录图标丢失的原因。
	 * 参考：FPP_ItemInfo::Icon 也是 TSoftObjectPtr<UTexture2D>，DT_Items 的图标能正常进包。
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "PP|Login")
	TSoftObjectPtr<UTexture2D> Icon;

	/**
	 * 图标路径字符串 —— 不用手填，由 UPP_MenuDataLibrary::GetLoginMenuItems 从 Icon 回填，
	 * Lua 侧（CommonBtn.lua）仍按 UE.UObject.Load(Obj.IconPath) 加载。
	 *
	 * 为什么不直接把 Icon 导出给 Lua：UnLuaEx 的 ADD_PROPERTY 走 UnLua::TType<T>/UnLua::Push<T>
	 * 模板，那边没有 TSoftObjectPtr 的特化，导出软引用成员编不过（见 PP_MenuData.cpp 的导出块）。
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Login")
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
class PERSONALPROJECTUI_API UPP_MenuDataLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 读取登录菜单的全部行，返回给 Lua 遍历
	 *
	 * @param InTable 登录菜单表（DT_LoginMenu）—— 由调用方传入（Lua 传 WBP_LoginView 的
	 *                LoginMenuTable 属性）。这里不再自己 LoadObject 按字符串路径加载：
	 *                那样加载的资产烘焙器看不见，打不进包。
	 */
	UFUNCTION(BlueprintCallable, Category = "PP|Login")
	static TArray<FLoginMenuItemRow> GetLoginMenuItems(UDataTable* InTable);
};
