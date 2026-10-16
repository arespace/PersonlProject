// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PP_LocalSaveGame.generated.h"

/**
 * UPP_LocalSaveGame - 本地存档（单个 Slot，存放与账号无关的本地偏好）
 *
 * 目前只记玩家昵称；以后设置屏的音量 / 画质 / 分辨率也可以往这里加字段，
 * 不用再新建存档类。
 *
 * 用法（一行读、一行写）：
 *   UPP_LocalSaveGame* Data = UPP_LocalSaveGame::LoadOrCreate();
 *   Data->PlayerName = TEXT("Cat");
 *   UPP_LocalSaveGame::Save(Data);
 */
UCLASS()
class PERSONALPROJECTCORE_API UPP_LocalSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	/** 上次使用的玩家昵称 */
	UPROPERTY(BlueprintReadWrite, Category = "PP|Save")
	FString PlayerName;

	/** 读取存档；没有或已损坏时返回一个新的空存档对象（不会是 nullptr） */
	UFUNCTION(BlueprintCallable, Category = "PP|Save")
	static UPP_LocalSaveGame* LoadOrCreate();

	/** 写回磁盘 */
	UFUNCTION(BlueprintCallable, Category = "PP|Save")
	static void Save(UPP_LocalSaveGame* SaveData);

private:
	/** 存档槽位名（UserIndex 固定 0） */
	static FString GetSlotName() { return TEXT("PP_LocalSettings"); }
};
