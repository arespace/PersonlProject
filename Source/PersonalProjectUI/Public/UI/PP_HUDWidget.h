// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UI/PP_UIUserWidget.h"
#include "GameplayTagContainer.h"
#include "UI/Quest/PP_QuestTrackerWidget.h"
#include "PP_HUDWidget.generated.h"

class UPP_UIViewModel;
class UPP_AttributeBarWidget;
class UPP_AbilitySlotWidget;
class UPP_CrosshairWidget;

/**
 * UPP_HUDWidget - 战斗HUD根节点
 * 持有ViewModel引用，通过BindWidget绑定蓝图子控件
 * 管理血蓝条、能力槽位、准星等子控件的生命周期
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_HUDWidget : public UPP_UIUserWidget
{
	GENERATED_BODY()

public:
	UPP_HUDWidget(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());
	/** 初始化HUD Widget，绑定ViewModel */
	void InitializeWidget(UPP_UIViewModel* InViewModel);

	/** 手动刷新所有属性条 */
	void UpdateAttributeBars() const;

	/** 初始化任务追踪器（绑定 QuestComponent 事件） */
	void InitializeQuestTracker();

protected:

	// ===== 蓝图绑定控件 - 在WBP_HUDWidget中放置同名控件 =====

	/** 生命值条 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPP_AttributeBarWidget> HealthBar;

	/** 法力值条 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPP_AttributeBarWidget> ManaBar;

	/** 能力槽位容器 - 存放能力槽位Widget */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPanelWidget> AbilitySlotContainer;

	/** 准星组件 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPP_CrosshairWidget> Crosshair;

	/** 任务追踪器 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UPP_QuestTrackerWidget> QuestTrackerWidget;

	// ===== 能力槽位配置 =====

	/** 能力槽位Widget类 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Ability")
	TSubclassOf<UPP_AbilitySlotWidget> AbilitySlotWidgetClass;

	/** 需要显示的能力Tag列表 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|HUD|Ability")
	TArray<FGameplayTag> AbilityTags;

private:

	/** ViewModel引用 */
	UPROPERTY()
	TObjectPtr<UPP_UIViewModel> ViewModel;

	/** 动态创建的能力槽位列表 */
	UPROPERTY()
	TArray<TObjectPtr<UPP_AbilitySlotWidget>> AbilitySlots;

	/** 创建能力槽位 */
	void CreateAbilitySlots();

	/** 绑定ViewModel委托 */
	void BindViewModelDelegates();

	/** ViewModel Health变更回调 */
	UFUNCTION()
	void OnHealthChanged(float NewValue, float MaxValue, float OldValue, float Delta);

	/** ViewModel Mana变更回调 */
	UFUNCTION()
	void OnManaChanged(float NewValue, float MaxValue, float OldValue, float Delta);
};
