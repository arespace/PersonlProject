// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_MenuData.h"
#include "UnLuaEx.h"

// 导出结构体到 UnLua，让 TArray<FLoginMenuItemRow> 的元素能以 Lua table 形式访问（.Name / .IconPath / .Action）
// 只导出 UnLua::TType<T> 有特化的类型（这里是 FString / FName）。
// Icon（TSoftObjectPtr）故意不导出：Lua 用不上它，它只负责把图标拉进烘焙依赖图；
// 硬导出去的话 ADD_PROPERTY 会走 UnLua::Push<TSoftObjectPtr<...>> 而没有对应特化，编不过。
BEGIN_EXPORT_REFLECTED_CLASS(FLoginMenuItemRow)
    ADD_PROPERTY(Name)
    ADD_PROPERTY(IconPath)
    ADD_PROPERTY(Action)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FLoginMenuItemRow)

TArray<FLoginMenuItemRow> UPP_MenuDataLibrary::GetLoginMenuItems(UDataTable* InTable)
{
	TArray<FLoginMenuItemRow> Result;

	if (!InTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuData] GetLoginMenuItems: InTable 为空，检查 WBP_LoginView 的 Class Defaults 里 LoginMenuTable 是否指到了 DT_LoginMenu"));
		return Result;
	}

	TArray<FLoginMenuItemRow*> Rows;
	InTable->GetAllRows<FLoginMenuItemRow>(TEXT("GetLoginMenuItems"), Rows);
	Result.Reserve(Rows.Num());
	for (const FLoginMenuItemRow* Row : Rows)
	{
		if (Row)
		{
			// Icon 是给烘焙器看的软引用，Lua 侧只认路径字符串，这里回填给 IconPath
			FLoginMenuItemRow Item = *Row;
			Item.IconPath = Row->Icon.ToSoftObjectPath().ToString();
			Result.Add(MoveTemp(Item));
		}
	}

	return Result;
}
