// Chen Tianbao uses GAS to create online battle games

#include "UI/PP_UIStyle.h"

FLinearColor UPP_UIStyle::GetGoldColor()   { return FLinearColor(0.910f, 0.769f, 0.478f, 1.0f); } // #E8C47A
FLinearColor UPP_UIStyle::GetAccentColor() { return FLinearColor(0.353f, 0.663f, 1.000f, 1.0f); } // #5AA9FF
FLinearColor UPP_UIStyle::GetReadyColor()  { return FLinearColor(0.275f, 0.827f, 0.541f, 1.0f); } // #46D38A
FLinearColor UPP_UIStyle::GetWarnColor()   { return FLinearColor(1.000f, 0.706f, 0.329f, 1.0f); } // #FFB454
FLinearColor UPP_UIStyle::GetDangerColor() { return FLinearColor(1.000f, 0.420f, 0.420f, 1.0f); } // #FF6B6B
FLinearColor UPP_UIStyle::GetMutedColor()  { return FLinearColor(0.541f, 0.580f, 0.659f, 1.0f); } // #8A94A8
FLinearColor UPP_UIStyle::GetTextColor()   { return FLinearColor(0.906f, 0.918f, 0.945f, 1.0f); } // #E7EAF1
FLinearColor UPP_UIStyle::GetDimColor()    { return FLinearColor(0.357f, 0.392f, 0.471f, 1.0f); } // #5B6478

int32 UPP_UIStyle::GetFontSizeSmall()    { return 11; }
int32 UPP_UIStyle::GetFontSizeBody()     { return 13; }
int32 UPP_UIStyle::GetFontSizeTitle()    { return 16; }
int32 UPP_UIStyle::GetFontSizeEmphasis() { return 20; }
