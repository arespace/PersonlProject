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

	/**
	 * 以【施法者】身份发放伤害 GE：伤害数值由 ExecCalc 结算，这里只传系数。
	 *
	 * 与 SendDamageEventToPlayer 的关键区别：spec 建在 Causer 的 ASC 上，使 ExecCalc
	 * 捕获 Source 属性时能解析到施法者（攻击力）。调用方负责判断目标是否存活。
	 *
	 * @param Causer       施法者（伤害来源，其 Attack 属性参与结算）
	 * @param Target       受击者（其 Defense 属性参与结算）
	 * @param DamageEffect 伤害 GE，其 Executions 需指向 UPP_DamageExecutionCalculation
	 * @param CoefficientTag SetByCaller 标签，需与 ExecCalc 的 CoefficientTag 一致
	 * @param Coefficient  伤害系数（正值，最终伤害 = 攻击力 × 系数 − 防御）
	 */
	UFUNCTION(BlueprintCallable, Category = "Crash|Abilities")
	static void ApplyDamageEffectFromCauser(AActor* Causer, AActor* Target, const TSubclassOf<UGameplayEffect>& DamageEffect,
		const FGameplayTag& CoefficientTag, float Coefficient);

	UFUNCTION(BlueprintCallable, Category= "Crash|Abilities")
	static TArray<AActor*> HitBoxOverlapTest(AActor* AvatarActor , float HitBoxRadius,float HitBoxForwardOffset = 0,float HitBoxElevationOffset = 0,bool bDrawDebugs = false);
	
	static void DrawHitBoxOverlapDebugs(const UObject* WorldContextObject, const TArray<FOverlapResult>& OverlapResults,const FVector& HitBoxLocation,float HitBoxRadius);
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Abilities")
	static TArray<AActor*> ApplyKnockback(AActor* AvatarActor, const TArray<AActor*>& HitActors, float InnerRadius, float OuterRadius, float LaunchForceMagnitude, float RotationAngle = 45.f, bool bDrawDebugs = false);

	/**
	 * 按接口能力查找 Actor 上的组件（返回第一个实现指定接口的组件，无则 nullptr）
	 * UI 等调用方借此避免「遍历 GetComponents + Implements」的重复样板
	 */
	static UObject* FindComponentImplementing(AActor* Owner, TSubclassOf<UInterface> InterfaceClass);
};
