// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_MenuData.h"
#include "UnLuaEx.h"

// 导出结构体到 UnLua，让 TArray<FLoginMenuItemRow> 的元素能以 Lua table 形式访问（.Name / .IconPath / .Action）
BEGIN_EXPORT_REFLECTED_CLASS(FLoginMenuItemRow)
    ADD_PROPERTY(Name)
    ADD_PROPERTY(IconPath)
    ADD_PROPERTY(Action)
END_EXPORT_CLASS()
IMPLEMENT_EXPORTED_CLASS(FLoginMenuItemRow)

TArray<FLoginMenuItemRow> UPP_MenuDataLibrary::GetLoginMenuItems()
{
	TArray<FLoginMenuItemRow> Result;

	UDataTable* DataTable = LoadObject<UDataTable>(nullptr, TEXT("/Game/PersonalProject/Data/Login/DT_LoginMenu.DT_LoginMenu"));
	if (!DataTable)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MenuData] Failed to load DT_LoginMenu, check path /Game/PersonalProject/Data/Login/DT_LoginMenu"));
		return Result;
	}

	TArray<FLoginMenuItemRow*> Rows;
	DataTable->GetAllRows<FLoginMenuItemRow>(TEXT("GetLoginMenuItems"), Rows);
	Result.Reserve(Rows.Num());
	for (const FLoginMenuItemRow* Row : Rows)
	{
		if (Row)
		{
			Result.Add(*Row);
		}
	}

	return Result;
}
