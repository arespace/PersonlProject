// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GameplayTagContainer.h"
#include "PP_AbilitySlotWidget.generated.h"

class UPP_UIViewModel;
class UImage;
class UTextBlock;
class USizeBox;

/**
 * UPP_AbilitySlotWidget - 能力槽位Widget
 * 支持Tag绑定与冷却监听，显示能力图标和冷却倒计时
 * 通过GameplayTag映射到对应的GAS能力
 */
UCLASS()
class PERSONALPROJECT_API UPP_AbilitySlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 初始化槽位，绑定到指定能力Tag */
	void InitializeSlot(UPP_UIViewModel* InViewModel, const FGameplayTag& InAbilityTag);

	/** 更新冷却状态 */
	void UpdateCooldown(float Remaining, float Duration);

	/** 获取绑定的能力Tag */
	FGameplayTag GetAbilityTag() const { return AbilityTag; }

protected:

	/** 蓝图绑定 - 能力图标 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> AbilityIcon;

	/** 蓝图绑定 - 冷却倒计时文本 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CooldownText;

	/** 蓝图绑定 - 冷却遮罩 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<USizeBox> CooldownOverlay;

	/** 绑定的能力Tag */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Ability")
	FGameplayTag AbilityTag;

	/** 蓝图实现事件 - 冷却更新 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Cooldown Updated"))
	void BP_OnCooldownUpdated(float Percent);

	/** 蓝图实现事件 - 能力激活 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Ability Activated"))
	void BP_OnAbilityActivated();

private:

	/** ViewModel引用 */
	UPROPERTY()
	TObjectPtr<UPP_UIViewModel> ViewModel;

	/** 冷却总时间 */
	float CooldownDuration = 0.f;

	/** 冷却剩余时间 */
	float CooldownRemaining = 0.f;

	/** 是否处于冷却中 */
	bool bIsOnCooldown = false;

	/** ViewModel冷却变更回调 */
	UFUNCTION()
	void OnCooldownChanged(FGameplayTag Tag, float RemainingTime, float Duration);
};
