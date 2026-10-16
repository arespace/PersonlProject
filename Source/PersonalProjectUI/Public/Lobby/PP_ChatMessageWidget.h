// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "PP_ChatMessageWidget.generated.h"

class UTextBlock;

/**
 * UPP_ChatMessageWidget - 大厅聊天单条消息
 *
 * 玩家消息："昵称：内容"；系统消息：金色整行（XX 加入了房间 / XX 创建了房间）。
 * 蓝图里摆放同名控件即可（BindWidgetOptional，缺了也不崩）：
 *   NameText     昵称（系统消息时留空）
 *   MessageText  消息内容
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_ChatMessageWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 普通聊天消息 */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby|Chat")
	void SetChatMessage(const FString& PlayerName, const FString& Message, bool bIsLocalPlayer);

	/** 系统提示消息（加入/创建房间等），整行高亮 */
	UFUNCTION(BlueprintCallable, Category = "PP|Lobby|Chat")
	void SetSystemMessage(const FString& Message);

protected:
	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Chat")
	TObjectPtr<UTextBlock> NameText;

	UPROPERTY(meta = (BindWidgetOptional), BlueprintReadOnly, Category = "PP|Lobby|Chat")
	TObjectPtr<UTextBlock> MessageText;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor NormalColor = FLinearColor(0.90f, 0.92f, 0.95f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor SelfColor = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);

	UPROPERTY(EditDefaultsOnly, Category = "PP|Lobby|Style")
	FLinearColor SystemColor = FLinearColor(1.0f, 0.84f, 0.0f, 1.0f);
};
