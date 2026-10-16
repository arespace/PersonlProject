// 物品 UI 相关蓝图函数库（Core 层）
// 从主玩法模块 Utils/PP_BlueprintLibrary 拆出：仅依赖 GameplayEffect，供 UI 模块调用，
// 避免 UI 为了读取 GE 属性摘要而 include 携带战斗逻辑的主玩法函数库。

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PP_ItemUILibrary.generated.h"

class UGameplayEffect;

/**
 * GE 属性修改器摘要 — 用于UI展示属性加成
 */
USTRUCT(BlueprintType)
struct FPP_ModifierSummary
{
	GENERATED_BODY()

	/** 属性显示名称（如 "攻击力"） */
	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	FText AttributeName;

	/** 修改器数值 */
	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	float ModifierValue = 0.f;

	/** 修改器操作类型文本（添加/乘算/覆写） */
	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	FText ModifierOpText;

	/** 用于排序和比较的内部标识 */
	UPROPERTY(BlueprintReadOnly, Category = "Modifier")
	FName AttributeIdentifier;
};

UCLASS()
class PERSONALPROJECTCORE_API UPP_ItemUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * 从 GameplayEffect 类中提取所有属性修改器摘要
	 * 用于UI提示框显示装备属性加成
	 */
	UFUNCTION(BlueprintCallable, Category = "PP|UI")
	static TArray<FPP_ModifierSummary> GetModifierSummaries(TSubclassOf<UGameplayEffect> EffectClass);

	/**
	 * 对比两件装备的属性差异
	 * @param CurrentEffectClass 当前已装备物品的 GE 类
	 * @param HoveredEffectClass 悬停物品的 GE 类
	 * @param OutBetter  输出：悬停装备更优的属性
	 * @param OutWorse   输出：悬停装备更差的属性
	 * @param OutNew     输出：悬停装备新增的属性（当前无但悬停有）
	 * @param OutLost    输出：悬停装备失去的属性（当前有但悬停无）
	 */
	UFUNCTION(BlueprintCallable, Category = "PP|UI")
	static void CompareModifiers(
		TSubclassOf<UGameplayEffect> CurrentEffectClass,
		TSubclassOf<UGameplayEffect> HoveredEffectClass,
		TArray<FPP_ModifierSummary>& OutBetter,
		TArray<FPP_ModifierSummary>& OutWorse,
		TArray<FPP_ModifierSummary>& OutNew,
		TArray<FPP_ModifierSummary>& OutLost
	);
};
