// 存档系统 — UI 向玩家控制器发起存档请求的统一接口
// UI 只依赖本接口（Core 模块），不直接引用 GameMode / 存档子系统 / 具体 PlayerController。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PP_SaveActions.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UPP_SaveActions : public UInterface
{
	GENERATED_BODY()
};

class PERSONALPROJECTCORE_API IPP_SaveActions
{
	GENERATED_BODY()

public:
	/** 请求保存当前进度：房主/单机直接写盘，客户端会转发给服务端（服务端只接受房主） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Save")
	void RequestSaveGame();

	/** 请求"继续游戏"：校验存档地图后 travel 到存档关卡（只有房主可用） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Save")
	void RequestLoadGame();

	/** 是否存在可继续的存档（主菜单按钮据此置灰） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Save")
	bool HasSaveGame() const;
};
