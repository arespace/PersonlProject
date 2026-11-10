// UI 消息网关接口 — 战斗 HUD 与大厅 HUD 共用的通知/聊天推送
// 单列的原因：两个 HUD 都需要消息能力，若并入战斗接口，大厅 HUD 会为两个方法被迫继承整套战斗声明。
// 实现方：APP_HUD、APP_LobbyHUD（PersonalProjectUI 模块）；调用方：APP_PlayerController。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PP_UIMessagingGateway.generated.h"

UINTERFACE(BlueprintType)
class UPP_UIMessagingGateway : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_UIMessagingGateway — UI 消息推送网关
 * 实现方：APP_HUD、APP_LobbyHUD
 */
class PERSONALPROJECT_API IPP_UIMessagingGateway
{
	GENERATED_BODY()

public:
	/** 系统通知（战斗 HUD 用于中央提示，大厅 HUD 可用于状态栏） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void PushNotification(const FString& Message);

	/** 聊天消息 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|UI")
	void PushChatMessage(const FString& PlayerName, const FString& Message);
};
