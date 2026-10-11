// Chen Tianbao uses GAS to create online battle games


#include "AbilitySystem/Executions/PP_DamageExecutionCalculation.h"

#include "AbilitySystem/PP_AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameplayTags/PPTags.h"

namespace
{
	/**
	 * 属性捕获定义。
	 * Attack 取 Source 并打快照 —— 出手瞬间定格，飞行途中给自己加 buff 不影响已发出的伤害；
	 * Defense 取 Target 且不取快照 —— 命中瞬间受击者的实时防御才生效。
	 */
	struct FPP_DamageStatics
	{
		DECLARE_ATTRIBUTE_CAPTUREDEF(Attack);
		DECLARE_ATTRIBUTE_CAPTUREDEF(Defense);

		FPP_DamageStatics()
		{
			DEFINE_ATTRIBUTE_CAPTUREDEF(UPP_AttributeSet, Attack, Source, true);
			DEFINE_ATTRIBUTE_CAPTUREDEF(UPP_AttributeSet, Defense, Target, false);
		}
	};

	const FPP_DamageStatics& DamageStatics()
	{
		static FPP_DamageStatics Statics;
		return Statics;
	}
}

UPP_DamageExecutionCalculation::UPP_DamageExecutionCalculation()
{
	// 未注册的捕获定义在结算时会被静默跳过，必须在这里登记
	RelevantAttributesToCapture.Add(DamageStatics().AttackDef);
	RelevantAttributesToCapture.Add(DamageStatics().DefenseDef);

	// 默认对齐火球现有约定；系数由 GA/投射物在发放伤害时用同一标签写入
	CoefficientTag = PPTags::SetByCaller::Player::Tertiary;
}

void UPP_DamageExecutionCalculation::Execute_Implementation(
	const FGameplayEffectCustomExecutionParameters& ExecutionParams,
	FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const
{
	const FGameplayEffectSpec& Spec = ExecutionParams.GetOwningSpec();

	FAggregatorEvaluateParameters EvaluateParameters;
	EvaluateParameters.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
	EvaluateParameters.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

	float Attack = 0.f;
	float Defense = 0.f;
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().AttackDef, EvaluateParameters, Attack);
	ExecutionParams.AttemptCalculateCapturedAttributeMagnitude(DamageStatics().DefenseDef, EvaluateParameters, Defense);

	// 系数取不到时退回 1.0（而非 0），避免标签配错时静默打出 0 伤害
	const float Coefficient = Spec.GetSetByCallerMagnitude(CoefficientTag, /*bWarnIfNotFound=*/false, 1.f);

	const float FinalDamage = FMath::Max(Attack * Coefficient - Defense, MinimumDamage);

	// 攻击力为 0 通常意味着 GE_InitializeAttributes 里漏配了 Attack 初值
	if (Attack <= 0.f)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("[PP_DamageExecution] 攻击力为 0，请检查 GE_InitializeAttributes 是否已为 Attack 配置初值（GE: %s）"),
			*GetNameSafe(Spec.Def));
	}

	UE_LOG(LogTemp, Log,
		TEXT("[PP_DamageExecution] Attack=%.2f Defense=%.2f Coefficient=%.2f FinalDamage=%.2f"),
		Attack, Defense, Coefficient, FinalDamage);

	// Health 采用负向修改量扣血，与项目既有约定一致
	OutExecutionOutput.AddOutputModifier(FGameplayModifierEvaluatedData(
		UPP_AttributeSet::GetHealthAttribute(), EGameplayModOp::Additive, -FinalDamage));
}
