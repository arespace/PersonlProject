// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PP_GameplayStatsLibrary.generated.h"

/**
 * 玩家运行时数值查询库 — C++ 暴露给 Lua 的数据接口
 *
 * 用途：Lua 侧 UI 通过 UE.UPP_GameplayStatsLibrary.* 读取实时游戏数值，
 *      实现「C++ 暴露接口 + Lua 调用 + 数据回传」的 Lua↔C++ 通信。
 * 数据源：PlayerController → PlayerState/Pawn → ASC → UPP_AttributeSet
 *        （与 UPP_UIViewModel 走同一条 GAS 链路，保证数值一致）
 */
UCLASS()
class PERSONALPROJECT_API UPP_GameplayStatsLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
};
