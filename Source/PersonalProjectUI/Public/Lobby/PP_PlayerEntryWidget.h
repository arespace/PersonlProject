// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Framework/PP_LobbyTypes.h"
#include "PP_PlayerEntryWidget.generated.h"

class UTextBlock;
class UImage;

/**
 * UPP_PlayerEntryWidget - 大厅玩家列表条目（替换原先直接往容器里塞 TextBlock 的写法）
 *
 * WBP_PlayerEntry 需要把 Parent Class 改成本类，并在蓝图里摆放同名控件：
 *   NameText    昵称
 *   ClassText   已选角色（SelectedCharacterId）
 *   StatusText  准备状态徽章
 *   HostIcon    房主皇冠图标（非房主时自动隐藏）
 *   AvatarImage 头像（可选）
 * 所有 BindWidgetOptional，缺哪个都不会崩。
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_PlayerEntryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 填充一名玩家的数据 */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby|Entry")
	void SetPlayerInfo(const FLobbyPlayerInfo& Info, bool bIsLocalPlayer);

	/** 填充为一个空位占位卡（补齐到 MaxPlayers，避免列表看起来残缺） */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby|Entry")
	void SetEmptySlot(int32 SlotIndex);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Entry")
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Entry")
	TObjectPtr<UTextBlock> ClassText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Entry")
	TObjectPtr<UTextBlock> StatusText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Entry")
	TObjectPtr<UImage> HostIcon;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Entry")
	TObjectPtr<UImage> AvatarImage;

	// ===== 样式令牌（蓝图里可覆盖，默认就是 V1.1 稿里那套配色） =====

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor HostColor = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor ReadyColor = FLinearColor(0.28f, 0.83f, 0.54f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor NormalColor = FLinearColor(0.90f, 0.92f, 0.95f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor EmptyColor = FLinearColor(0.35f, 0.39f, 0.47f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor SelfColor = FLinearColor(0.35f, 0.66f, 1.0f, 1.0f);
};
