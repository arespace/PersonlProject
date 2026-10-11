// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "GameplayTagContainer.h"

#include "PP_DamageExecutionCalculation.generated.h"

/**
 * 伤害结算：最终伤害 = 攻击力 × 系数 − 防御
 *
 * 攻击力从 Source（施法者）捕获并打快照，防御力从 Target（受击者）实时捕获。
 * 系数通过 SetByCaller 传入，使同一个 ExecCalc + 同一个 GE 能服务多个技能。
 *
 * 注意：Source 必须能被正确解析成施法者，因此发放伤害时要把 spec 建在【施法者】
 * 的 ASC 上（见 UPP_BlueprintLibrary::ApplyDamageEffectFromCauser）。若建在受击者
 * 的 ASC 上，Source 会解析成受击者自己，攻击力将被读成敌人的攻击力。
 */
UCLASS()
class PERSONALPROJECT_API UPP_DamageExecutionCalculation : public UGameplayEffectExecutionCalculation
{
	GENERATED_BODY()

public:
	UPP_DamageExecutionCalculation();

	virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams,
		FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

	/** SetByCaller 系数标签：系数由技能/投射物在发放伤害时写入 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Damage")
	FGameplayTag CoefficientTag;

	/** 防御减免后的伤害下限，避免高防御目标完全免疫 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Damage", meta = (ClampMin = "0.0"))
	float MinimumDamage{1.f};
};
