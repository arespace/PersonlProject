// Chen Tianbao uses GAS to create online battle games

#include "UI/Widgets/PP_ScoreboardWidget.h"

#include "Components/PanelWidget.h"

UPP_ScoreboardWidget::UPP_ScoreboardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Overlay;
}

void UPP_ScoreboardWidget::UpdateScoreboard(const TArray<FScoreboardEntry>& Entries)
{
	// 通知蓝图更新视觉
	BP_OnScoreboardUpdated(Entries);

	// 如果配置了容器和条目类，动态创建条目
	if (ScoreboardContainer && ScoreboardEntryClass)
	{
		ScoreboardContainer->ClearChildren();

		for (const FScoreboardEntry& Entry : Entries)
		{
			UUserWidget* EntryWidget = CreateWidget<UUserWidget>(this, ScoreboardEntryClass);
			if (EntryWidget)
			{
				ScoreboardContainer->AddChild(EntryWidget);
			}
		}
	}
}
