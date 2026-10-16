// Chen Tianbao uses GAS to create online battle games


#include "Player/PP_PlayerState.h"

#include "AbilitySystem/PP_AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Quest/PP_QuestComponent.h"
#include "Net/UnrealNetwork.h"

APP_PlayerState::APP_PlayerState()
{
	SetNetUpdateFrequency(100.f);
	
	AbilitySystemComponent = CreateDefaultSubobject<UPP_AbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);
	
	AttributeSet = CreateDefaultSubobject<UPP_AttributeSet>("AttributeSet");

	// 库存组件挂载在 PlayerState 上，确保 SeamlessTravel 后数据保留
	InventoryComponent = CreateDefaultSubobject<UPP_InventoryComponent>("InventoryComponent");
	InventoryComponent->SetIsReplicated(true);

	// 任务组件挂载在 PlayerState 上
	QuestComponent = CreateDefaultSubobject<UPP_QuestComponent>("QuestComponent");
	QuestComponent->SetIsReplicated(true);
}

UAbilitySystemComponent* APP_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void APP_PlayerState::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(APP_PlayerState, SelectedCharacterId);
	// InventoryComponent 通过组件自身的复制机制同步内部状态，组件指针本身随 PlayerState 自动复制，无需在此再 DOREPLIFETIME
}
