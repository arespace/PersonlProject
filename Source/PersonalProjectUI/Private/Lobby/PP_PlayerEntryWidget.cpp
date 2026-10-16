// Chen Tianbao uses GAS to create online battle games

#include "Lobby/PP_PlayerEntryWidget.h"
#include "Components/TextBlock.h"
#include "Components/Image.h"

void UPP_PlayerEntryWidget::SetPlayerInfo(const FLobbyPlayerInfo& Info, bool bIsLocalPlayer)
{
	// 昵称
	if (NameText)
	{
		NameText->SetText(FText::FromString(Info.PlayerName));
		// 颜色优先级：房主 > 自己 > 普通
		if (Info.bIsHost)
		{
			NameText->SetColorAndOpacity(FSlateColor(HostColor));
		}
		else if (bIsLocalPlayer)
		{
			NameText->SetColorAndOpacity(FSlateColor(SelfColor));
		}
		else
		{
			NameText->SetColorAndOpacity(FSlateColor(NormalColor));
		}
	}

	// 已选角色（未选时给灰色提示，而不是留空）
	if (ClassText)
	{
		if (Info.SelectedCharacterId.IsNone())
		{
			ClassText->SetText(FText::FromString(TEXT("\u672a\u9009\u62e9")));
			ClassText->SetColorAndOpacity(FSlateColor(EmptyColor));
		}
		else
		{
			ClassText->SetText(FText::FromName(Info.SelectedCharacterId));
			ClassText->SetColorAndOpacity(FSlateColor(NormalColor));
		}
	}

	// 准备状态徽章
	if (StatusText)
	{
		if (Info.bIsReady)
		{
			StatusText->SetText(FText::FromString(TEXT("\u5df2\u51c6\u5907")));
			StatusText->SetColorAndOpacity(FSlateColor(ReadyColor));
		}
		else
		{
			StatusText->SetText(FText::FromString(TEXT("\u672a\u51c6\u5907")));
			StatusText->SetColorAndOpacity(FSlateColor(EmptyColor));
		}
	}

	// 房主皇冠：非房主直接隐藏，而不是换个灰图标
	if (HostIcon)
	{
		HostIcon->SetVisibility(Info.bIsHost ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Visible);
}

void UPP_PlayerEntryWidget::SetEmptySlot(int32 SlotIndex)
{
	if (NameText)
	{
		NameText->SetText(FText::FromString(TEXT("+ \u7a7a\u4f4d")));
		NameText->SetColorAndOpacity(FSlateColor(EmptyColor));
	}
	if (ClassText)
	{
		ClassText->SetText(FText::FromString(FString::Printf(TEXT("#%d"), SlotIndex + 1)));
		ClassText->SetColorAndOpacity(FSlateColor(EmptyColor));
	}
	if (StatusText)
	{
		StatusText->SetText(FText::FromString(TEXT("\u7b49\u5f85\u52a0\u5165")));
		StatusText->SetColorAndOpacity(FSlateColor(EmptyColor));
	}
	if (HostIcon)
	{
		HostIcon->SetVisibility(ESlateVisibility::Collapsed);
	}

	SetVisibility(ESlateVisibility::Visible);
}
