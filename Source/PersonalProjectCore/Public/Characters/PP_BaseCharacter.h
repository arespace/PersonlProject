// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "AlsCharacter.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"

#include "PP_BaseCharacter.generated.h"

namespace PersonalTags
{
	extern PERSONALPROJECTCORE_API const FName Player;
}

class UAttributeSet;
class UGameplayAbility;
struct FOnAttributeChangeData;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FASCInitialized, UAbilitySystemComponent*, ASC, UAttributeSet*, AS);

UCLASS(Abstract)
class PERSONALPROJECTCORE_API APP_BaseCharacter : public AAlsCharacter, public IAbilitySystemInterface 
{
	GENERATED_BODY()

public:
	APP_BaseCharacter();
	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const {return nullptr;};
	bool IsAlive() const {return bAlive;}
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }
	
	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Death")
	virtual void HandleRespawn();
	
	UFUNCTION(BlueprintCallable, Category = "Crash|Attributes")
	void ResetAttributes();
	
	UFUNCTION(BlueprintImplementableEvent)
	void RotateToTarget(AActor* RotateTarget);
	
	UPROPERTY(EditAnywhere, Category = "Crash|AI")
	float SearchRange{1000.f};

	// 是否强制套用 ALS 动画蓝图 AB_Als。
	// ALS Tick 有一条守卫：没有 UAlsAnimationInstance 就整体提前 return（含 RefreshView/相机刷新），
	// 因此需要 ALS 相机与视角的角色（玩家）必须为 true；
	// 使用自带动画蓝图且不需要 ALS 相机的角色（敌人）保持 false，避免骨架不匹配导致动作别扭。
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Als")
	bool bForceAlsAnimationBlueprint{false};
protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

	void GiveStartupAbilities();
	void InitializeAttributes() const;
	
	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	virtual void HandleDeath();

	// 受击反应：临时切换到 ALS Injured 覆盖层，短暂后恢复 Default。
	// 死亡（血量归零）走 HandleDeath→StartRagdolling，不触发受击反应。
	virtual void PlayHitReaction();
	void ResetHitReactionOverlay();
	FTimerHandle HitReactionTimerHandle;
	
private:
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;
	
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;
	
	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Replicated)
	bool bAlive = true;
	
};
