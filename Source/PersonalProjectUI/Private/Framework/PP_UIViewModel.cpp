// Chen Tianbao uses GAS to create online battle games

#include "Framework/PP_UIViewModel.h"

#include "AbilitySystem/PP_AbilitySystemComponent.h"
#include "AbilitySystem/PP_AttributeSet.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"

void UPP_UIViewModel::Initialize(APlayerController* InOwner)
{
	if (!IsValid(InOwner))
	{
		return;
	}

	OwnerController = InOwner;

	// 从PlayerState获取ASC引用
	if (APlayerState* PS = InOwner->GetPlayerState<APlayerState>())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(PS))
		{
			ASC = Cast<UPP_AbilitySystemComponent>(ASI->GetAbilitySystemComponent());
		}
	}

	// 如果PlayerState上没有ASC，尝试从Pawn获取
	if (!ASC.IsValid() && InOwner->GetPawn())
	{
		if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(InOwner->GetPawn()))
		{
			ASC = Cast<UPP_AbilitySystemComponent>(ASI->GetAbilitySystemComponent());
		}
	}

	if (ASC.IsValid())
	{
		// GetAttributeSet返回const指针，需要去除const以赋值给TWeakObjectPtr
		if (const UAttributeSet* RawAS = ASC->GetAttributeSet(UPP_AttributeSet::StaticClass()))
		{
			AttributeSet = const_cast<UPP_AttributeSet*>(Cast<UPP_AttributeSet>(RawAS));
		}
		BindToASC();
	}
}

void UPP_UIViewModel::Deinitialize()
{
	// 清除ASC属性变更委托绑定
	if (ASC.IsValid() && AttributeSet.IsValid())
	{
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetManaAttribute()).RemoveAll(this);
		ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxManaAttribute()).RemoveAll(this);
	}

	// 清除UI委托
	OnHealthChanged.Clear();
	OnManaChanged.Clear();
	OnDamageReceived.Clear();
	OnPlayerDeath.Clear();
	OnPlayerRespawn.Clear();

	ASC.Reset();
	AttributeSet.Reset();
	OwnerController.Reset();
}

float UPP_UIViewModel::GetHealth() const
{
	if (AttributeSet.IsValid())
	{
		return AttributeSet->GetHealth();
	}
	return CachedHealth;
}

float UPP_UIViewModel::GetMaxHealth() const
{
	if (AttributeSet.IsValid())
	{
		return AttributeSet->GetMaxHealth();
	}
	return CachedMaxHealth;
}

float UPP_UIViewModel::GetMana() const
{
	if (AttributeSet.IsValid())
	{
		return AttributeSet->GetMana();
	}
	return CachedMana;
}

float UPP_UIViewModel::GetMaxMana() const
{
	if (AttributeSet.IsValid())
	{
		return AttributeSet->GetMaxMana();
	}
	return CachedMaxMana;
}

bool UPP_UIViewModel::IsAlive() const
{
	return GetHealth() > 0.f;
}

float UPP_UIViewModel::GetAbilityCooldownRemaining(const FGameplayTag& AbilityTag) const
{
	if (!ASC.IsValid())
	{
		return 0.f;
	}

	// 使用ASC的冷却查询接口
	return ASC->GetCooldownRemainingForTag(AbilityTag);
}

float UPP_UIViewModel::GetAbilityCooldownPercent(const FGameplayTag& AbilityTag) const
{
	if (!ASC.IsValid())
	{
		return 0.f;
	}

	const float Remaining = ASC->GetCooldownRemainingForTag(AbilityTag);
	const float Duration = ASC->GetCooldownDurationForTag(AbilityTag);

	if (Duration <= 0.f)
	{
		return 0.f;
	}

	return FMath::Clamp(Remaining / Duration, 0.f, 1.f);
}

void UPP_UIViewModel::BindToASC()
{
	if (!ASC.IsValid() || !AttributeSet.IsValid())
	{
		return;
	}

	// 缓存初始属性值
	CachedHealth = AttributeSet->GetHealth();
	CachedMaxHealth = AttributeSet->GetMaxHealth();
	CachedMana = AttributeSet->GetMana();
	CachedMaxMana = AttributeSet->GetMaxMana();

	// 绑定Health属性变更
	ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		.AddUObject(this, &UPP_UIViewModel::OnHealthAttributeChanged);

	// 绑定MaxHealth属性变更
	ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxHealthAttribute())
		.AddUObject(this, &UPP_UIViewModel::OnMaxHealthAttributeChanged);

	// 绑定Mana属性变更
	ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetManaAttribute())
		.AddUObject(this, &UPP_UIViewModel::OnManaAttributeChanged);

	// 绑定MaxMana属性变更
	ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetMaxManaAttribute())
		.AddUObject(this, &UPP_UIViewModel::OnMaxManaAttributeChanged);
}

void UPP_UIViewModel::OnHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	const float OldValue = CachedHealth;
	CachedHealth = Data.NewValue;
	const float Delta = Data.NewValue - OldValue;

	// 广播Health变更
	OnHealthChanged.Broadcast(Data.NewValue, CachedMaxHealth, OldValue, Delta);

	// 如果Health降为0，触发死亡事件
	if (Data.NewValue <= 0.f && OldValue > 0.f)
	{
		if (OwnerController.IsValid())
		{
			OnPlayerDeath.Broadcast(OwnerController->GetPawn());
		}
	}
	// 如果Health从0恢复，触发复活事件（对称：HUD 收到后 HideDeathScreen → 恢复游戏输入）
	else if (Data.NewValue > 0.f && OldValue <= 0.f)
	{
		OnPlayerRespawn.Broadcast();
	}

	// 广播伤害/治疗事件用于伤害数字
	if (Delta != 0.f)
	{
		FVector ActorLocation = FVector::ZeroVector;
		if (OwnerController.IsValid())
		{
			if (APawn* Pawn = OwnerController->GetPawn())
			{
				ActorLocation = Pawn->GetActorLocation();
			}
		}

		const bool bIsHeal = (Delta > 0.f);
		const bool bIsCritical = false;
		OnDamageReceived.Broadcast(FMath::Abs(Delta), ActorLocation, bIsHeal, bIsCritical, nullptr);
	}
}

void UPP_UIViewModel::OnManaAttributeChanged(const FOnAttributeChangeData& Data)
{
	const float OldValue = CachedMana;
	CachedMana = Data.NewValue;
	const float Delta = Data.NewValue - OldValue;

	OnManaChanged.Broadcast(Data.NewValue, CachedMaxMana, OldValue, Delta);
}

void UPP_UIViewModel::OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxHealth = Data.NewValue;

	// MaxHealth变更时也重新广播Health，因为比例可能变化
	OnHealthChanged.Broadcast(CachedHealth, CachedMaxHealth, CachedHealth, 0.f);
}

void UPP_UIViewModel::OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data)
{
	CachedMaxMana = Data.NewValue;

	// MaxMana变更时也重新广播Mana
	OnManaChanged.Broadcast(CachedMana, CachedMaxMana, CachedMana, 0.f);
}
