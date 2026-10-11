// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "PP_LoginViewWidget.generated.h"

class UDataTable;

/**
 * UPP_LoginViewWidget - WBP_LoginView 的 C++ 基类（登录界面，独立屏幕空间面板）
 *
 * 为什么需要这个类：
 *   登录菜单的数据表与条目类原来是在 Lua 里按字符串路径加载的
 *   （UE.UClass.Load('/Game/UI/UIObj/BP_ItemData.BP_ItemData_C')），
 *   C++ 侧 GetLoginMenuItems() 里也同样用 LoadObject 硬编码了 DT_LoginMenu 的路径。
 *   烘焙器（cooker）沿引用图决定哪些资产进包，**字符串路径它看不见**，
 *   所以打包后 DT_LoginMenu / BP_ItemData 根本没进包，登录界面菜单项全空。
 *   把这两个资产放成 Widget 上的属性引用后，烘焙器顺着 WBP_LoginView 的 CDO 就能跟到它们。
 *
 * 使用步骤：
 *   1) 在编辑器里把 WBP_LoginView 的父类改成 UPP_LoginViewWidget（Reparent Blueprint）；
 *   2) 打开 WBP_LoginView 的 Class Defaults，设置：
 *        LoginMenuTable → /Game/PersonalProject/Data/Login/DT_LoginMenu
 *        MenuItemClass  → /Game/UI/UIObj/BP_ItemData
 */
UCLASS(BlueprintType, Blueprintable)
class PERSONALPROJECT_API UPP_LoginViewWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	/** 登录菜单数据表（DT_LoginMenu）—— 硬引用，烘焙器一定会把它打进包 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Login")
	TObjectPtr<UDataTable> LoginMenuTable;

	/**
	 * 菜单条目数据对象类（BP_ItemData）—— 硬引用，烘焙器一定会把它打进包
	 *
	 * 注意这里是 TSubclassOf<UObject> 而不是 TSubclassOf<UUserWidget>：
	 * BP_ItemData 的父类是 Object（只实现了 UserObjectListEntry + UnLuaInterface 两个接口），
	 * 它不是 UUserWidget，用 TSubclassOf<UUserWidget> 在编辑器里根本选不中这个类。
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Login")
	TSubclassOf<UObject> MenuItemClass;
};
