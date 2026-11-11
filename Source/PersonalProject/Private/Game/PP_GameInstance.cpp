#include "Game/PP_GameInstance.h"

#include "CoreGlobals.h"
#include "Engine/World.h"
#include "Framework/PP_Log.h"
#include "Game/PP_CrashGameMode.h"
#include "Game/Save/PP_SaveSubsystem.h"
#include "Misc/App.h"

UPP_GameInstance* UPP_GameInstance::Get(const UWorld* World)
{
	if (!World) return nullptr;
	return Cast<UPP_GameInstance>(World->GetGameInstance());
}

void UPP_GameInstance::Shutdown()
{
	// 退出游戏保存：在 Super::Shutdown() 之前做，此时 World / PlayerController 仍然有效
	//（基类会先移除 LocalPlayer，之后再调用 IsLocalController 就取不到本机玩家了）。
	// PIE 不写档，避免编辑器里反复 Play 污染本地存档；专用服务器没有本地玩家，直接跳过。
	if (!GIsPlayInEditorWorld && !IsRunningDedicatedServer())
	{
		if (UPP_SaveSubsystem* SaveSubsystem = GetSubsystem<UPP_SaveSubsystem>())
		{
			UWorld* World = GetWorld();

			// 只在大厅以外的战斗地图保存：大厅没有可存的进度；
			// 读档尚未应用（PendingLoad）时也不写，避免用空进度覆盖好档。
			const bool bInBattleMap = World && World->GetAuthGameMode<APP_CrashGameMode>() != nullptr;
			if (bInBattleMap && !SaveSubsystem->HasPendingLoad())
			{
				FString Error;
				if (SaveSubsystem->SaveLocalPlayer(Error))
				{
					UE_LOG(LogPP, Log, TEXT("[PP_Save] 退出游戏自动保存完成"));
				}
				else
				{
					UE_LOG(LogPP, Warning, TEXT("[PP_Save] 退出游戏自动保存失败：%s"), *Error);
				}
			}
		}
	}

	Super::Shutdown();
}
