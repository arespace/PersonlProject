// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/PP_AttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/PPTags.h"
#include"Net/UnrealNetwork.h"

void UPP_AttributeSet::GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,Health,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,MaxHealth,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,Mana,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,MaxMana,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,Attack,COND_None,REPNOTIFY_Always);
	DOREPLIFETIME_CONDITION_NOTIFY(ThisClass,Defense,COND_None,REPNOTIFY_Always);

	DOREPLIFETIME(ThisClass,bAttributesInitialized);
}

void UPP_AttributeSet::PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute())
	{
		// PostGameplayEffectExecute 触发时，本次 GE 的 Modifier 已经作用到 Health 上了
		// （引擎 GameplayEffect.cpp 的 InternalExecuteMod：先 ApplyModToAttribute，再回调），
		// 所以 GetHealth() 是"扣完血、尚未 Clamp"的值，减去 Magnitude 即还原出变化前的血量。
		const float HealthBefore = GetHealth() - Data.EvaluatedData.Magnitude;
		SetHealth(FMath::Clamp(GetHealth(), 0.f, GetMaxHealth()));

		// 伤害/治疗日志：Magnitude 为负 = 掉血，为正 = 回血。
		// 施法者取 EffectContext 的 SourceObject —— 两条伤害链路都会写入它：
		//   ApplyDamageEffectFromCauser（火球）写 Causer；SendDamageEventToPlayer（敌人）写 Payload.Instigator
		const float Delta = Data.EvaluatedData.Magnitude;
		const TCHAR* Verb = (Delta < 0.f) ? TEXT("受到伤害") : TEXT("恢复");

		UE_LOG(LogTemp, Log, TEXT("[PP_Health] %s %s %.2f | Health %.2f → %.2f | 施法者: %s | GE: %s"),
			*GetNameSafe(Data.Target.GetAvatarActor()),
			Verb,
			FMath::Abs(Delta),
			HealthBefore,
			GetHealth(),
			*GetNameSafe(Data.EffectSpec.GetEffectContext().GetSourceObject()),
			*GetNameSafe(Data.EffectSpec.Def));
	}
	
	if (Data.EvaluatedData.Attribute == GetManaAttribute())
	{
		SetMana(FMath::Clamp(GetMana(), 0.f, GetMaxMana()));
	}
	
	if (Data.EvaluatedData.Attribute == GetHealthAttribute() && GetHealth() <= 0.f)
	{
			
			FGameplayEventData Payload;
			Payload.Instigator = Data.Target.GetAvatarActor();
			UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Data.EffectSpec.GetEffectContext().GetInstigator(), PPTags::Events::KillScored, Payload);
	}
	
	if (!bAttributesInitialized)
	{
		// ===================== 临时诊断：把生效 GE 的 Modifier 逐条打出来，定位完删除 =====================
		// 属性值最终是 0、但编辑器那一行"看着配好了"的情况只剩两种：
		//   ① ModifierOp 不是 Additive —— 基础值为 0 时 Multiplicitive(0×n) 与 Division(0/n) 结果都还是 0
		//   ② ModifierMagnitude 的量值本身算出来就是 0 —— SetByCaller 没写标签，或 AttributeBased 依赖的属性也是 0
		// （"Attribute 指向 None / FFieldPath 解析失败"已被排除：那条路会在 InternalExecuteMod:4038
		//   的 GetAttributeSetClass() 里直接 check 崩溃，而不是静默跳过 —— 没崩就说明 Attribute 有效。）
		// AttemptCalculateMagnitude 拿的是真实 Spec，会把 SetByCaller/AttributeBased 一并解析，
		// 所以"解析失败"和"解析出来就是 0"在这一行里能分开看。
		if (const UGameplayEffect* ActiveGE = Data.EffectSpec.Def)
		{
			UE_LOG(LogTemp, Warning, TEXT("[PP_InitDiag] GE=%s  共 %d 条 Modifier"),
				*GetNameSafe(ActiveGE), ActiveGE->Modifiers.Num());

			for (int32 ModIndex = 0; ModIndex < ActiveGE->Modifiers.Num(); ++ModIndex)
			{
				const FGameplayModifierInfo& ModInfo = ActiveGE->Modifiers[ModIndex];

				float StaticValue = -1.f;
				const bool bHasStaticValue = ModInfo.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, StaticValue);

				float ResolvedValue = -1.f;
				const bool bResolved = ModInfo.ModifierMagnitude.AttemptCalculateMagnitude(
					Data.EffectSpec, ResolvedValue, /*WarnIfSetByCallerFail=*/false);

				UE_LOG(LogTemp, Warning,
					TEXT("[PP_InitDiag]  #%d 属性='%s' 有效=%d | Op=%d | 量值类型=%d | 静态值OK=%d 值=%.2f | 解析OK=%d 值=%.2f"),
					ModIndex,
					*ModInfo.Attribute.GetName(),
					ModInfo.Attribute.IsValid() ? 1 : 0,
					(int32)ModInfo.ModifierOp.GetValue(),
					(int32)ModInfo.ModifierMagnitude.GetMagnitudeCalculationType(),
					bHasStaticValue ? 1 : 0, StaticValue,
					bResolved ? 1 : 0, ResolvedValue);
			}
		}
		// ===================== 临时诊断结束 =====================

		bAttributesInitialized = true;	
		OnAttributesInitialized.Broadcast();
	}
}

void UPP_AttributeSet::OnRep_AttributesInitalized()
{
	if (bAttributesInitialized)
	{
		OnAttributesInitialized.Broadcast();
	}
}

void UPP_AttributeSet::OnRep_Health(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,Health,OldValue);
}

void UPP_AttributeSet::OnRep_MaxHealth(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,MaxHealth,OldValue);
}

void UPP_AttributeSet::OnRep_Mana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,Mana,OldValue);
}

void UPP_AttributeSet::OnRep_MaxMana(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,MaxMana,OldValue);
}

void UPP_AttributeSet::OnRep_Attack(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,Attack,OldValue);
}

void UPP_AttributeSet::OnRep_Defense(const FGameplayAttributeData& OldValue)
{
	GAMEPLAYATTRIBUTE_REPNOTIFY(ThisClass,Defense,OldValue);
}
