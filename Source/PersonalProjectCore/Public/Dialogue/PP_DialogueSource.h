// NPC对话系统 — 对话数据源接口
// 解耦 UI 对 APP_NPCActor（Quest 模块）具体类的直接持有：
// UI 通过本接口查询对话表与商店数据源，无需知道对话发起方的具体类型。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PP_DialogueSource.generated.h"

class UDataTable;

UINTERFACE(BlueprintType)
class UPP_DialogueSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_DialogueSource — 对话数据源能力接口
 * 实现方：APP_NPCActor（Quest 模块）
 * 调用方：UI 模块（对话面板、选项面板、HUD 网关实现）
 *
 * 约定：本接口只读；接口方法名用 Query 前缀，避免与实现类已有 UFUNCTION 同名冲突。
 */
class PERSONALPROJECTCORE_API IPP_DialogueSource
{
	GENERATED_BODY()

public:
	/** 查询对话数据表（FPP_DialogueData 行结构；未配置返回 nullptr） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Source")
	UDataTable* QueryDialogueTable();

	/** 查询商店数据源（实现 IPP_ShopQuery / IPP_ShopActions 的对象；无商店返回 nullptr） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Dialogue|Source")
	UObject* QueryShopSource();
};
