
#include "Widgets/PP_ScoreboardRowWidget.h"

#include "Components/TextBlock.h"

void UPP_ScoreboardRowWidget::SetEntry(const FScoreboardEntry& InEntry)
{
	Entry = InEntry;

	// 逐个绑定判空：绑了就填，没绑就记录控件名，最后统一提示
	TArray<FString> MissingWidgets;

	if (Text_Name)
	{
		Text_Name->SetText(FText::FromString(Entry.PlayerName));
	}
	else
	{
		MissingWidgets.Add(TEXT("Text_Name"));
	}

	if (Text_Kills)
	{
		Text_Kills->SetText(FText::AsNumber(Entry.Kills));
	}
	else
	{
		MissingWidgets.Add(TEXT("Text_Kills"));
	}

	if (Text_Deaths)
	{
		Text_Deaths->SetText(FText::AsNumber(Entry.Deaths));
	}
	else
	{
		MissingWidgets.Add(TEXT("Text_Deaths"));
	}

	if (Text_Score)
	{
		Text_Score->SetText(FText::AsNumber(Entry.Score));
	}
	else
	{
		MissingWidgets.Add(TEXT("Text_Score"));
	}

	if (MissingWidgets.Num() == 4)
	{
		// 四个都没绑：只打一条汇总，避免每次刷新刷四条日志
		UE_LOG(LogTemp, Warning, TEXT("[PP_ScoreboardRowWidget] 未绑定任何文本控件，已跳过文本填充。期望控件名：Text_Name / Text_Kills / Text_Deaths / Text_Score"));
	}
	else
	{
		for (const FString& WidgetName : MissingWidgets)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PP_ScoreboardRowWidget] 未绑定文本控件 %s，对应字段不会显示"), *WidgetName);
		}
	}

	// 保留蓝图事件：默认文本已填好，蓝图可在此做换色、插图标等额外定制
	OnEntrySet(Entry);
}
