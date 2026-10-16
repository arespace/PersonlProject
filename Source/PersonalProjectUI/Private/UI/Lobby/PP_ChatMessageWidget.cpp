// Chen Tianbao uses GAS to create online battle games

#include "UI/Lobby/PP_ChatMessageWidget.h"
#include "Components/TextBlock.h"

void UPP_ChatMessageWidget::SetChatMessage(const FString& PlayerName, const FString& Message, bool bIsLocalPlayer)
{
	if (NameText)
	{
		NameText->SetVisibility(ESlateVisibility::Visible);
		NameText->SetText(FText::FromString(FString::Printf(TEXT("%s\uff1a"), *PlayerName)));
		NameText->SetColorAndOpacity(FSlateColor(bIsLocalPlayer ? SelfColor : NormalColor));
	}

	if (MessageText)
	{
		MessageText->SetText(FText::FromString(Message));
		MessageText->SetColorAndOpacity(FSlateColor(NormalColor));
	}
}

void UPP_ChatMessageWidget::SetSystemMessage(const FString& Message)
{
	if (NameText)
	{
		// 系统消息不需要昵称前缀，直接隐藏而不是留空文本，避免多余空格
		NameText->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (MessageText)
	{
		MessageText->SetText(FText::FromString(FString::Printf(TEXT("\u2014 %s \u2014"), *Message)));
		MessageText->SetColorAndOpacity(FSlateColor(SystemColor));
	}
}
