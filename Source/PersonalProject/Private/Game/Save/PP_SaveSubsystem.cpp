#include "Game/Save/PP_SaveSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "Characters/PP_PlayerCharacter.h"
#include "Framework/PP_Log.h"
#include "GameFramework/PlayerController.h"
#include "Inventory/PP_InventoryComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Player/PP_PlayerState.h"
#include "Save/PP_SaveGame.h"

namespace
{
	/** 读取九项属性的 base 值（不含装备 GE 的 Modifier） */
	void CaptureAttributes(UAbilitySystemComponent* ASC, FPP_SaveAttributeData& Out)
	{
		if (!ASC)
		{
			return;
		}

		Out.Health = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetHealthAttribute());
		Out.MaxHealth = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetMaxHealthAttribute());
		Out.Mana = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetManaAttribute());
		Out.MaxMana = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetMaxManaAttribute());
		Out.Stamina = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetStaminaAttribute());
		Out.MaxStamina = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetMaxStaminaAttribute());
		Out.Attack = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetAttackAttribute());
		Out.Defense = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetDefenseAttribute());
		Out.StaminaRegenRate = ASC->GetNumericAttributeBase(UPP_AttributeSet::GetStaminaRegenRateAttribute());
	}

	/** 写回九项属性的 base 值：先写上限，再把当前值夹到上限内 */
	void ApplyAttributes(UAbilitySystemComponent* ASC, const FPP_SaveAttributeData& In)
	{
		if (!ASC)
		{
			return;
		}

		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetMaxHealthAttribute(), In.MaxHealth);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetMaxManaAttribute(), In.MaxMana);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetMaxStaminaAttribute(), In.MaxStamina);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetAttackAttribute(), In.Attack);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetDefenseAttribute(), In.Defense);
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetStaminaRegenRateAttribute(), In.StaminaRegenRate);

		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetHealthAttribute(), FMath::Clamp(In.Health, 0.f, In.MaxHealth));
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetManaAttribute(), FMath::Clamp(In.Mana, 0.f, In.MaxMana));
		ASC->SetNumericAttributeBase(UPP_AttributeSet::GetStaminaAttribute(), FMath::Clamp(In.Stamina, 0.f, In.MaxStamina));
	}
}

UPP_SaveSubsystem* UPP_SaveSubsystem::Get(const UObject* WorldContextObject)
{
	UGameInstance* GameInstance = UGameplayStatics::GetGameInstance(WorldContextObject);
	return GameInstance ? GameInstance->GetSubsystem<UPP_SaveSubsystem>() : nullptr;
}

bool UPP_SaveSubsystem::SavePlayer(APlayerController* PC, FString& OutError)
{
	OutError.Reset();

	if (!PC || !PC->HasAuthority())
	{
		OutError = TEXT("只能在服务端保存");
		return false;
	}

	APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>();
	if (!PS)
	{
		OutError = TEXT("未找到 PlayerState");
		return false;
	}

	UPP_SaveGame* SaveData = Cast<UPP_SaveGame>(UGameplayStatics::CreateSaveGameObject(UPP_SaveGame::StaticClass()));
	if (!SaveData)
	{
		OutError = TEXT("创建存档对象失败");
		return false;
	}

	SaveData->MapName = FName(*UGameplayStatics::GetCurrentLevelName(PC->GetWorld(), true));
	SaveData->PlayerName = PS->GetPlayerName();
	SaveData->Character.CharacterId = PS->GetSelectedCharacterId();
	CaptureAttributes(PS->GetAbilitySystemComponent(), SaveData->Character.BaseAttributes);

	if (const UPP_InventoryComponent* Inventory = PS->GetInventoryComponent())
	{
		SaveData->Inventory.Items = Inventory->GetItems();
		SaveData->Inventory.Gold = Inventory->GetGold();

		// 装备只落盘 ItemID + Slot；EffectHandle 是运行时句柄，读档时重新解析重建
		SaveData->Inventory.Equipped = Inventory->GetEquippedItems();
		for (FPP_EquippedItem& Equipped : SaveData->Inventory.Equipped)
		{
			Equipped.EffectHandle = FActiveGameplayEffectHandle();
		}
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 保存时未找到 InventoryComponent，背包数据未写入"));
	}

	return UPP_SaveGame::Save(SaveData, OutError);
}

bool UPP_SaveSubsystem::LoadFromDisk(FString& OutError)
{
	UPP_SaveGame* Loaded = UPP_SaveGame::Load(OutError);
	if (!Loaded)
	{
		if (OutError.IsEmpty())
		{
			OutError = TEXT("没有存档");
		}
		return false;
	}

	PendingSave = Loaded;
	UE_LOG(LogPP, Log, TEXT("[PP_Save] 已读取存档到内存：地图=%s 角色=%s"),
		*Loaded->MapName.ToString(), *Loaded->Character.CharacterId.ToString());
	return true;
}

bool UPP_SaveSubsystem::ApplyPendingToPlayer(APlayerController* PC, FString& OutError)
{
	OutError.Reset();

	if (!PC || !PC->HasAuthority())
	{
		OutError = TEXT("只能在服务端读档");
		return false;
	}

	if (!PendingSave)
	{
		OutError = TEXT("没有待应用的存档");
		return false;
	}

	APP_PlayerState* PS = PC->GetPlayerState<APP_PlayerState>();
	if (!PS)
	{
		OutError = TEXT("未找到 PlayerState");
		return false;
	}

	const FPP_SaveCharacterData& Character = PendingSave->Character;

	PS->SetSelectedCharacterId(Character.CharacterId);

	if (APP_PlayerCharacter* PlayerCharacter = Cast<APP_PlayerCharacter>(PC->GetPawn()))
	{
		PlayerCharacter->ApplyCharacterAppearance(Character.CharacterId);
	}

	// 先恢复背包/装备：装备 GE 的加成先挂上去，最后再写属性 base，避免加成被 base 覆盖或双算
	if (UPP_InventoryComponent* Inventory = PS->GetInventoryComponent())
	{
		Inventory->RestoreFromSave(PendingSave->Inventory);
	}
	else
	{
		UE_LOG(LogPP, Warning, TEXT("[PP_Save] 读档时未找到 InventoryComponent，背包数据未应用"));
	}

	ApplyAttributes(PS->GetAbilitySystemComponent(), Character.BaseAttributes);

	UE_LOG(LogPP, Log, TEXT("[PP_Save] 已应用存档到 %s：角色=%s 生命=%.1f/%.1f 法力=%.1f/%.1f 耐力=%.1f/%.1f"),
		*PS->GetPlayerName(), *Character.CharacterId.ToString(),
		Character.BaseAttributes.Health, Character.BaseAttributes.MaxHealth,
		Character.BaseAttributes.Mana, Character.BaseAttributes.MaxMana,
		Character.BaseAttributes.Stamina, Character.BaseAttributes.MaxStamina);

	// one-shot：应用成功即清除，重生/重连/再次 Possess 不得重复套用
	PendingSave = nullptr;
	return true;
}
