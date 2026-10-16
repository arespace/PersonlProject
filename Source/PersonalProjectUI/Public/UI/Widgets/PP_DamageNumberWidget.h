// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"
#include "PP_DamageNumberWidget.generated.h"

/** 伤害数字动画结束委托 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDamageNumberFinished, UPP_DamageNumberWidget*, Widget);

/**
 * UPP_DamageNumberWidget - 伤害数字弹出Widget
 * 实现世界坐标到屏幕坐标的投影，显示伤害/治疗数值
 * 配合 UIManager 对象池实现复用
 */
UCLASS()
class PERSONALPROJECTUI_API UPP_DamageNumberWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/** 配置伤害数字参数 */
	void Configure(float Damage, FVector WorldLocation, bool bIsHeal, bool bIsCritical);

	/** 动画是否播放完毕 */
	bool IsFinished() const { return bAnimationFinished; }

	/** 动画结束委托（池管理器监听以回收 Widget） */
	UPROPERTY(BlueprintAssignable, Category = "PP|DamageNumber")
	FOnDamageNumberFinished OnFinished;

protected:

	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	/** 蓝图绑定 - 伤害数值文本 */
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> DamageText;

	/** 蓝图实现事件 - 伤害数字配置完成后调用，用于触发动画 */
	UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = "On Damage Configured"))
	void BP_OnDamageConfigured(float Damage, bool bIsHeal, bool bIsCritical);

private:

	/** 缓存的世界坐标位置 */
	FVector CachedWorldPosition = FVector::ZeroVector;

	/** 显示时间（秒） */
	float DisplayTime = 0.f;

	/** 最大显示时间（秒） */
	static constexpr float MaxDisplayTime = 1.5f;

	/** 垂直飘动速度（像素/秒） */
	static constexpr float FloatSpeed = 50.f;

	/** 动画是否完成 */
	bool bAnimationFinished = false;
};
