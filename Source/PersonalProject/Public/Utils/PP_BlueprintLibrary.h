// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PP_BlueprintLibrary.generated.h"


UENUM(BlueprintType)
enum class EHitDirection : uint8
{
	Left,
	Right,
	Forward,
	Back
};


USTRUCT(BlueprintType)
struct FClosestActorWithTagResult
{
	GENERATED_BODY();
	
	UPROPERTY(BlueprintReadWrite)
	TWeakObjectPtr<AActor> Actor;
	
	UPROPERTY(BlueprintReadWrite)
	float Distance{0.f};
};

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
class PERSONALPROJECT_API UPP_BlueprintLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
	
public:
	
	UFUNCTION(BlueprintPure)
	static EHitDirection GetHitDirection(const FVector& TargetForward, const FVector& ToInstigator);
	
	UFUNCTION(BlueprintPure)
	static FName GetHitDirectionName(const EHitDirection& HitDirection);
	
	UFUNCTION(BlueprintCallable)
	static FClosestActorWithTagResult FindClosestActorWithTag(UObject* WorldContextObject,const FVector& Origin , const FName& Tag,float SearchRange);
	
	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayer(AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect, UPARAM(ref) FGameplayEventData& Payload, const FGameplayTag& DataTag
		, float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem = nullptr);

	UFUNCTION(BlueprintCallable)
	static void SendDamageEventToPlayers(TArray<AActor*> Targets, const TSubclassOf<UGameplayEffect>& DamageEffect, UPARAM(ref) FGameplayEventData& Payload, const FGameplayTag& DataTag, float Damage, const FGameplayTag& EventTagOverride, UObject* OptionalParticleSystem = nullptr);

	UFUNCTION(BlueprintCallable, Category= "Crash|Abilities")
	static TArray<AActor*> HitBoxOverlapTest(AActor* AvatarActor , float HitBoxRadius,float HitBoxForwardOffset = 0,float HitBoxElevationOffset = 0,bool bDrawDebugs = false);
	
	static void DrawHitBoxOverlapDebugs(const UObject* WorldContextObject, const TArray<FOverlapResult>& OverlapResults,const FVector& HitBoxLocation,float HitBoxRadius);
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Abilities")
	static TArray<AActor*> ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors, float InnerRadius, float OuterRadius, float LaunchForceMagnitude, float RotationAngle = 45.f, bool bDrawDebugs = false);

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
