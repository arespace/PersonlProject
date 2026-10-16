// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "GameplayTagContainer.h"
#include "PP_UIViewModel.generated.h"

class UPP_AbilitySystemComponent;
class UPP_AttributeSet;
class APlayerController;
struct FOnAttributeChangeData;

/** 属性变更事件委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FOnUIAttributeChanged, float, NewValue, float, MaxValue, float, OldValue, float, Delta);

/** 伤害接收事件委托 - 用于伤害数字 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnDamageReceived, float, Damage, FVector, WorldLocation, bool, bIsHeal, bool, bIsCritical, AActor*, SourceActor);

/** 玩家死亡委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPlayerDeath, AActor*, DeadActor);

/** 玩家复活委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnPlayerRespawn);

/**
 * UPP_UIViewModel - 数据中介层
 * 统一封装GAS数据访问，UI Widget不直接接触ASC，通过ViewModel间接访问
 * 职责：ASC属性监听、冷却查询、伤害事件广播、状态同步
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_UIViewModel : public UObject
{
	GENERATED_BODY()

public:
	/** 初始化ViewModel，绑定到PlayerController的ASC */
	void Initialize(APlayerController* InOwner);

	/** 反初始化，清除所有绑定 */
	void Deinitialize();

	// ===== 属性查询接口 =====

	/** 获取当前生命值 */
	float GetHealth() const;

	/** 获取最大生命值 */
	float GetMaxHealth() const;

	/** 获取当前法力值 */
	float GetMana() const;

	/** 获取最大法力值 */
	float GetMaxMana() const;

	/** 玩家是否存活 */
	bool IsAlive() const;

	// ===== 委托广播 - UI订阅这些委托获取数据更新 =====

	/** 生命值变更时广播 */
	UPROPERTY(BlueprintAssignable)
	FOnUIAttributeChanged OnHealthChanged;

	/** 法力值变更时广播 */
	UPROPERTY(BlueprintAssignable)
	FOnUIAttributeChanged OnManaChanged;

	/** 接收伤害时广播 - 伤害数字Widget使用 */
	UPROPERTY(BlueprintAssignable)
	FOnDamageReceived OnDamageReceived;

	/** 玩家死亡时广播 */
	UPROPERTY(BlueprintAssignable)
	FOnPlayerDeath OnPlayerDeath;

	/** 玩家复活时广播 */
	UPROPERTY(BlueprintAssignable)
	FOnPlayerRespawn OnPlayerRespawn;

private:
	/** 弱引用ASC，避免循环引用 */
	TWeakObjectPtr<UPP_AbilitySystemComponent> ASC;

	/** 弱引用AttributeSet */
	TWeakObjectPtr<UPP_AttributeSet> AttributeSet;

	/** 弱引用所属PlayerController */
	TWeakObjectPtr<APlayerController> OwnerController;

	/** 绑定ASC属性变更委托 */
	void BindToASC();

	/** Health属性变更回调 */
	void OnHealthAttributeChanged(const FOnAttributeChangeData& Data);

	/** Mana属性变更回调 */
	void OnManaAttributeChanged(const FOnAttributeChangeData& Data);

	/** MaxHealth属性变更回调 */
	void OnMaxHealthAttributeChanged(const FOnAttributeChangeData& Data);

	/** MaxMana属性变更回调 */
	void OnMaxManaAttributeChanged(const FOnAttributeChangeData& Data);

	/** 缓存当前属性值，避免频繁访问ASC */
	float CachedHealth = 0.f;
	float CachedMaxHealth = 0.f;
	float CachedMana = 0.f;
	float CachedMaxMana = 0.f;
};
