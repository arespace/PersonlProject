
#include "Widgets/PP_ScoreboardRowWidget.h"

void UPP_ScoreboardRowWidget::SetEntry(const FScoreboardEntry& InEntry)
{
	Entry = InEntry;
	OnEntrySet(Entry);
}
