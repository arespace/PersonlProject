// 物品 UI 相关蓝图函数库（Core 层）

#include "Framework/PP_ItemUILibrary.h"
#include "GameplayEffect.h"

TArray<FPP_ModifierSummary> UPP_ItemUILibrary::GetModifierSummaries(TSubclassOf<UGameplayEffect> EffectClass)
{
	TArray<FPP_ModifierSummary> Result;

	if (!EffectClass)
	{
		return Result;
	}

	// 获取 GE 类的默认对象以读取 Modifiers
	const UGameplayEffect* DefaultGE = EffectClass->GetDefaultObject<UGameplayEffect>();
	if (!DefaultGE)
	{
		return Result;
	}

	for (const FGameplayModifierInfo& Modifier : DefaultGE->Modifiers)
	{
		if (!Modifier.Attribute.IsValid())
		{
			continue;
		}

		FPP_ModifierSummary Summary;
		Summary.AttributeIdentifier = FName(*Modifier.Attribute.AttributeName);
		Summary.AttributeName = FText::FromString(Modifier.Attribute.AttributeName);
		Summary.ModifierValue = 0.f;

		// 尝试读取 Magnitude 的静态值
		if (Modifier.ModifierMagnitude.GetStaticMagnitudeIfPossible(1.f, Summary.ModifierValue))
		{
			// ModifierValue 已从 Magnitude 读取
		}
		else
		{
			// 无法获取静态值（可能使用了 SetByCaller），设为0
			Summary.ModifierValue = 0.f;
		}

		// 操作类型文本
		switch (Modifier.ModifierOp)
		{
		case EGameplayModOp::Additive:
			Summary.ModifierOpText = NSLOCTEXT("PP", "ModOpAdd", "添加");
			break;
		case EGameplayModOp::Multiplicitive:
			Summary.ModifierOpText = NSLOCTEXT("PP", "ModOpMult", "乘算");
			break;
		case EGameplayModOp::Division:
			Summary.ModifierOpText = NSLOCTEXT("PP", "ModOpDiv", "除算");
			break;
		case EGameplayModOp::Override:
			Summary.ModifierOpText = NSLOCTEXT("PP", "ModOpOver", "覆写");
			break;
		default:
			Summary.ModifierOpText = FText::GetEmpty();
			break;
		}

		Result.Add(Summary);
	}

	return Result;
}

void UPP_ItemUILibrary::CompareModifiers(
	TSubclassOf<UGameplayEffect> CurrentEffectClass,
	TSubclassOf<UGameplayEffect> HoveredEffectClass,
	TArray<FPP_ModifierSummary>& OutBetter,
	TArray<FPP_ModifierSummary>& OutWorse,
	TArray<FPP_ModifierSummary>& OutNew,
	TArray<FPP_ModifierSummary>& OutLost)
{
	OutBetter.Reset();
	OutWorse.Reset();
	OutNew.Reset();
	OutLost.Reset();

	TArray<FPP_ModifierSummary> CurrentMods = GetModifierSummaries(CurrentEffectClass);
	TArray<FPP_ModifierSummary> HoveredMods = GetModifierSummaries(HoveredEffectClass);

	// 构建当前装备的属性索引
	TMap<FName, float> CurrentMap;
	for (const FPP_ModifierSummary& Mod : CurrentMods)
	{
		CurrentMap.Add(Mod.AttributeIdentifier, Mod.ModifierValue);
	}

	// 构建悬停装备的属性索引
	TMap<FName, float> HoveredMap;
	for (const FPP_ModifierSummary& Mod : HoveredMods)
	{
		HoveredMap.Add(Mod.AttributeIdentifier, Mod.ModifierValue);
	}

	// 遍历悬停装备的属性，与当前装备对比
	for (FPP_ModifierSummary& HoveredMod : HoveredMods)
	{
		if (float* CurrentVal = CurrentMap.Find(HoveredMod.AttributeIdentifier))
		{
			if (HoveredMod.ModifierValue > *CurrentVal)
			{
				OutBetter.Add(HoveredMod);
			}
			else if (HoveredMod.ModifierValue < *CurrentVal)
			{
				OutWorse.Add(HoveredMod);
			}
			// 相等则不输出到任何列表
		}
		else
		{
			OutNew.Add(HoveredMod);
		}
	}

	// 当前装备有而悬停装备没有的 → Lost
	for (FPP_ModifierSummary& CurrentMod : CurrentMods)
	{
		if (!HoveredMap.Contains(CurrentMod.AttributeIdentifier))
		{
			OutLost.Add(CurrentMod);
		}
	}
}
