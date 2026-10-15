// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PP_UIStyle.generated.h"

/**
 * UPP_UIStyle - 大厅 UI 样式令牌
 *
 * 为什么要有它：原先颜色散落在 C++ 里写死（例如 PP_LobbyWidget 中的
 * FLinearColor(1.0f, 0.84f, 0.0f)），改一次配色要翻遍所有类。
 * 现在统一从这里取，蓝图也能直接调，改配色只改这一个地方。
 */
UCLASS()
class PERSONALPROJECT_API UPP_UIStyle : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 主色（金）#E8C47A —— 房主、强调、主按钮 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetGoldColor();

	/** 辅助色（蓝）#5AA9FF —— 自己、链接、昵称 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetAccentColor();

	/** 成功（绿）#46D38A —— 已准备 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetReadyColor();

	/** 警告（橙）#FFB454 —— 房主迁移、网络波动 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetWarnColor();

	/** 危险（红）#FF6B6B —— 离开、踢人、错误 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetDangerColor();

	/** 次要文本灰 #8A94A8 —— 说明文字、空位 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetMutedColor();

	/** 正文文字色 #E7EAF1 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetTextColor();

	/** 淡出灰（用于未配置/禁用占位）#5B6478 */
	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static FLinearColor GetDimColor();

	// ===== 字号四档 =====

	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static int32 GetFontSizeSmall();   // 11 辅助

	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static int32 GetFontSizeBody();    // 13 正文

	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static int32 GetFontSizeTitle();   // 16 小标题

	UFUNCTION(BlueprintPure, Category = "PP|UIStyle")
	static int32 GetFontSizeEmphasis();// 20 强调
};
