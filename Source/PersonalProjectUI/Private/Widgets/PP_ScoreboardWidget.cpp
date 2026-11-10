
#include "Widgets/PP_ScoreboardWidget.h"

#include "Components/PanelWidget.h"
#include "Widgets/PP_ScoreboardRowWidget.h"

UPP_ScoreboardWidget::UPP_ScoreboardWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Overlay;
	// 计分板是纯展示型 Widget（按 Tab 显示时不该拦截视角移动），不消费输入模式
	bConsumesAllInput = false;
}

void UPP_ScoreboardWidget::UpdateScoreboard(const TArray<FScoreboardEntry>& Entries)
{
	// 保留原有蓝图事件：已连线的蓝图逻辑不受本次改造影响
	BP_OnScoreboardUpdated(Entries);

	if (!ScoreboardContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PP_ScoreboardWidget] 未绑定 ScoreboardContainer，无法创建计分板行；请在 WBP_Scoreboard 中保留名为 ScoreboardContainer 的容器控件"));
		return;
	}

	// 每次更新先清空已有行，既避免重复叠加，也保证空列表能正确清空
	ScoreboardContainer->ClearChildren();

	if (!ScoreboardEntryClass)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PP_ScoreboardWidget] 未配置 ScoreboardEntryClass，计分板不会动态创建行（蓝图更新事件仍会触发）"));
		return;
	}

	for (const FScoreboardEntry& Entry : Entries)
	{
		UPP_ScoreboardRowWidget* Row = CreateWidget<UPP_ScoreboardRowWidget>(this, ScoreboardEntryClass);
		if (!Row)
		{
			continue;
		}

		// 先加入容器让行 Widget 完成 Construct，再写入数据触发 OnEntrySet 刷新显示
		ScoreboardContainer->AddChild(Row);
		Row->SetEntry(Entry);
	}
}
