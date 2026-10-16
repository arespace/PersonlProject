// Chen Tianbao uses GAS to create online battle games

#include "HUD/PP_HUDWidget.h"

#include "Framework/PP_UIViewModel.h"
#include "HUD/PP_AttributeBarWidget.h"
#include "Widgets/PP_AbilitySlotWidget.h"
#include "Widgets/PP_CrosshairWidget.h"
#include "Quest/PP_QuestTrackerWidget.h"
#include "Quest/PP_QuestComponent.h"
#include "Player/PP_PlayerState.h"
#include "Components/PanelWidget.h"
#include "GameplayTags/PPTags.h"
#include "Framework/PP_Log.h"

UPP_HUDWidget::UPP_HUDWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	OwningLayer = EPP_UILayer::Gameplay;
}

void UPP_HUDWidget::InitializeWidget(UPP_UIViewModel* InViewModel)
{
	ViewModel = InViewModel;

	// 如果没有配置AbilityTags，使用默认的三个能力Tag
	if (AbilityTags.Num() == 0)
	{
		AbilityTags.Add(PPTags::PPAbilities::Primary);
		AbilityTags.Add(PPTags::PPAbilities::Secondary);
		AbilityTags.Add(PPTags::PPAbilities::Tertiary);
	}

	CreateAbilitySlots();
	BindViewModelDelegates();
	UpdateAttributeBars();
	InitializeQuestTracker();
}

void UPP_HUDWidget::UpdateAttributeBars() const
{
	if (!ViewModel)
	{
		return;
	}

	if (HealthBar)
	{
		HealthBar->UpdateValue(ViewModel->GetHealth(), ViewModel->GetMaxHealth());
	}

	if (ManaBar)
	{
		ManaBar->UpdateValue(ViewModel->GetMana(), ViewModel->GetMaxMana());
	}
}

void UPP_HUDWidget::CreateAbilitySlots()
{
	if (!AbilitySlotContainer || !AbilitySlotWidgetClass)
	{
		return;
	}

	AbilitySlotContainer->ClearChildren();
	AbilitySlots.Empty();

	for (const FGameplayTag& Tag : AbilityTags)
	{
		UPP_AbilitySlotWidget* AbilitySlot = CreateWidget<UPP_AbilitySlotWidget>(this, AbilitySlotWidgetClass);
		if (AbilitySlot)
		{
			AbilitySlot->InitializeSlot(Tag);
			AbilitySlotContainer->AddChild(AbilitySlot);
			AbilitySlots.Add(AbilitySlot);
		}
	}
}

void UPP_HUDWidget::BindViewModelDelegates()
{
	if (!ViewModel)
	{
		return;
	}

	ViewModel->OnHealthChanged.AddDynamic(this, &UPP_HUDWidget::OnHealthChanged);
	ViewModel->OnManaChanged.AddDynamic(this, &UPP_HUDWidget::OnManaChanged);
}

void UPP_HUDWidget::OnHealthChanged(float NewValue, float MaxValue, float OldValue, float Delta)
{
	if (HealthBar)
	{
		HealthBar->UpdateValue(NewValue, MaxValue);
	}
}

void UPP_HUDWidget::OnManaChanged(float NewValue, float MaxValue, float OldValue, float Delta)
{
	if (ManaBar)
	{
		ManaBar->UpdateValue(NewValue, MaxValue);
	}
}

void UPP_HUDWidget::InitializeQuestTracker()
{
	if (!QuestTrackerWidget)
	{
		UE_LOG(LogPP, Error, TEXT("[HUDWidget] InitializeQuestTracker - QuestTrackerWidget is NULL (BindWidget failed)"));
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC)
	{
		UE_LOG(LogPP, Warning, TEXT("[HUDWidget] InitializeQuestTracker - No owning player"));
		return;
	}

	APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>();
	if (!PS)
	{
		UE_LOG(LogPP, Warning, TEXT("[HUDWidget] InitializeQuestTracker - No PlayerState"));
		return;
	}

	UPP_QuestComponent* QuestComp = PS->GetQuestComponent();
	if (!QuestComp)
	{
		UE_LOG(LogPP, Warning, TEXT("[HUDWidget] InitializeQuestTracker - No QuestComponent on PlayerState"));
		return;
	}

	UE_LOG(LogPP, Log, TEXT("[HUDWidget] InitializeQuestTracker - SUCCESS, ActiveQuests=%d"), QuestComp->GetActiveQuests().Num());
	QuestTrackerWidget->InitializeTracker(QuestComp);
}
