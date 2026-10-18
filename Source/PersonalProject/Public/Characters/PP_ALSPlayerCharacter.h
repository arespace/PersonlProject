// ALS + GAS 集成 POC：继承 AAlsCharacterExample 并接入 GAS 的玩家角色
// AAlsCharacterExample 提供 ALS 相机组件、输入绑定、CalcCamera 等完整 ALS 玩法骨架，
// 本类只需在其基础上叠加 GAS（StartupAbilities / 两个 GE / OnASCInitialized /
// GiveStartupAbilities / InitializeAttributes 等）。
// 由于 C++ 不允许多继承两个 UCLASS，无法同时继承 APP_BaseCharacter，
// 因此将 PP_BaseCharacter 的 GAS 样板移植到本类。
// POC 阶段 ASC 直接挂在角色上（对齐 PP_EnemyCharacter 模式），无需依赖 APP_PlayerState。

#pragma once

#include "CoreMinimal.h"
#include "AlsCharacterExample.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "Characters/PP_BaseCharacter.h"
#include "AbilitySystem/PP_AttributeSet.h"

#include "PP_ALSPlayerCharacter.generated.h"

class UPP_AbilitySystemComponent;
class UGameplayAbility;
struct FOnAttributeChangeData;

UCLASS()
class PERSONALPROJECT_API APP_ALSPlayerCharacter : public AAlsCharacterExample, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	APP_ALSPlayerCharacter();

	virtual void GetLifetimeReplicatedProps(TArray<class FLifetimeProperty>& OutLifetimeProps) const override;
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const { return AttributeSet; }

	bool IsAlive() const { return bAlive; }
	void SetAlive(bool bAliveStatus) { bAlive = bAliveStatus; }

	UPROPERTY(BlueprintAssignable)
	FASCInitialized OnASCInitialized;

	UFUNCTION(BlueprintCallable, Category = "Crash|Death")
	virtual void HandleRespawn();

	UFUNCTION(BlueprintCallable, Category = "Crash|Attributes")
	void ResetAttributes();

protected:
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;

	void GiveStartupAbilities();
	void InitializeAttributes() const;

	void OnHealthChanged(const FOnAttributeChangeData& AttributeChangeData);
	virtual void HandleDeath();

private:
	UPROPERTY(EditDefaultsOnly, Category = "Crash|Abilities")
	TArray<TSubclassOf<UGameplayAbility>> StartupAbilities;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> InitializeAttributesEffect;

	UPROPERTY(EditDefaultsOnly, Category = "Crash|Effects")
	TSubclassOf<UGameplayEffect> ResetAttributesEffect;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crash|Abilities", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_AbilitySystemComponent> AbilitySystemComponent;

	UPROPERTY()
	TObjectPtr<UPP_AttributeSet> AttributeSet;

	UPROPERTY(BlueprintReadOnly, meta = (AllowPrivateAccess = "true"), Replicated)
	bool bAlive = true;
};
