#include "Characters/PP_WeaponAbilityComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Data/PP_ItemTypes.h"
#include "Framework/PP_DataManager.h"
#include "Framework/PP_Log.h"
#include "GameFramework/Pawn.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Player/PP_PlayerState.h"

UPP_WeaponAbilityComponent::UPP_WeaponAbilityComponent()
{
	// GA 只在装备变化时切换，不需要每帧更新
	PrimaryComponentTick.bCanEverTick = false;
}

UAbilitySystemComponent* UPP_WeaponAbilityComponent::GetOwnerAbilitySystemComponent() const
{
	// ASC 在 PlayerState 上，角色通过 IAbilitySystemInterface 转发，这里按接口取即可
	const IAbilitySystemInterface* AbilitySystemInterface = Cast<IAbilitySystemInterface>(GetOwner());
	return AbilitySystemInterface ? AbilitySystemInterface->GetAbilitySystemComponent() : nullptr;
}

void UPP_WeaponAbilityComponent::ApplyEquippedWeapon(const EPP_EquipSlot Slot, const FName NewItemID)
{
	// GA 的授予/移除必须由权威端执行
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	GrantWeaponAbility(Slot, NewItemID);
}

void UPP_WeaponAbilityComponent::RefreshAbilityFromInventory()
{
	// 客户端不授予 GA，由服务器授予后复制
	if (!IsValid(GetOwner()) || !GetOwner()->HasAuthority())
	{
		return;
	}

	const APawn* Pawn = Cast<APawn>(GetOwner());
	APP_PlayerState* PPPlayerState = IsValid(Pawn) ? Pawn->GetPlayerState<APP_PlayerState>() : nullptr;
	UPP_InventoryComponent* Inventory = IsValid(PPPlayerState) ? PPPlayerState->GetInventoryComponent() : nullptr;
	if (!IsValid(Inventory))
	{
		return;
	}

	// 单武器槽口径：只恢复主手
	const FName WeaponID = Inventory->GetEquippedItemID(EPP_EquipSlot::MainHand);
	if (WeaponID.IsNone())
	{
		return;
	}

	// 本生命周期内已授予过则跳过，避免重复实例
	if (!WeaponAbilityHandles.Contains(EPP_EquipSlot::MainHand))
	{
		GrantWeaponAbility(EPP_EquipSlot::MainHand, WeaponID);
	}
}

void UPP_WeaponAbilityComponent::GrantWeaponAbility(const EPP_EquipSlot Slot, const FName NewItemID)
{
	UAbilitySystemComponent* ASC = GetOwnerAbilitySystemComponent();
	if (!IsValid(ASC))
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] GrantWeaponAbility: ASC 无效，无法切换武器 GA"));
		return;
	}

	// 第一步：移除该槽位上一次激活的 GA
	if (const FGameplayAbilitySpecHandle* OldHandle = WeaponAbilityHandles.Find(Slot))
	{
		if (OldHandle->IsValid())
		{
			ASC->ClearAbility(*OldHandle);
			WeaponAbilityHandles.Remove(Slot);
		}
	}

	// 第二步：如果 NewItemID 有效 → 查武器配置 → Give 新 GA
	if (NewItemID.IsNone())
	{
		// 卸下，到此结束
		return;
	}

	// 唯一数据源：DT_Items 行内嵌的 WeaponConfig
	UPP_DataManager* DataManager = UPP_DataManager::Get(GetWorld());
	const FPP_WeaponConfig* WeaponConfig = DataManager ? DataManager->GetWeaponConfig(NewItemID) : nullptr;
	if (!WeaponConfig)
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] ItemID=%s 在 DT_Items 中没有 WeaponConfig，不授予武器 GA"), *NewItemID.ToString());
		return;
	}

	const TSubclassOf<UGameplayAbility> AbilityClass = WeaponConfig->WeaponAbilityClass;
	if (!AbilityClass)
	{
		// 该武器未配置 GA 类，跳过（纯属性武器）
		return;
	}

	// GiveAbility（Level 1，无 InputBinding）；SourceObject 仍是角色，保持与迁移前一致
	FGameplayAbilitySpec Spec(AbilityClass, 1, INDEX_NONE, GetOwner());
	const FGameplayAbilitySpecHandle NewHandle = ASC->GiveAbility(Spec);
	if (NewHandle.IsValid())
	{
		WeaponAbilityHandles.Add(Slot, NewHandle);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[WeaponGA] 槽 %d 装备 %s → GiveAbility 失败"),
			static_cast<int32>(Slot), *NewItemID.ToString());
	}
}
