// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PP_AbilitySlotWidget.generated.h"

class UImage;

/**
 * UPP_AbilitySlotWidget - 能力槽位Widget
 * 支持Tag绑定，显示能力图标
 * 通过GameplayTag映射到对应的GAS能力
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_AbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化槽位，绑定到指定能力Tag */
	void InitializeSlot(const FGameplayTag& InAbilityTag);

	/** 获取绑定的能力Tag */
	FGameplayTag GetAbilityTag() const { return AbilityTag; }

protected:

	/** 蓝图绑定 - 能力图标 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon;

	/** 绑定的能力Tag */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Ability")
	FGameplayTag AbilityTag;

	/** 蓝图实现事件 - 能力激活 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Ability Activated"))
	void BP_OnAbilityActivated();
};
