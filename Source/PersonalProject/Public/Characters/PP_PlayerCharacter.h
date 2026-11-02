// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Characters/PP_BaseCharacter.h"
#include "Characters/PP_CharacterData.h"
#include "PP_PlayerCharacter.generated.h"

struct FInputActionValue;
class UAlsCameraComponent;
class UInputMappingContext;
class UInputAction;

UCLASS()
class PERSONALPROJECT_API APP_PlayerCharacter : public APP_BaseCharacter
{
	GENERATED_BODY()

public:
	APP_PlayerCharacter();

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;
	virtual UAttributeSet* GetAttributeSet() const override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;
	virtual void Tick(float DeltaTime) override;

	// 玩家死亡后由服务端计时器复活（基类 HandleDeath 启动）
	virtual bool ShouldRespawn() const override { return true; }
	virtual void ServerRespawn() override;

public:
	/** 根据角色ID应用外观（模型+动画） */
	void ApplyCharacterAppearance(FName CharacterId);

	// ===== ALS 相机与输入（移植自 AAlsCharacterExample，避免依赖 demo 模块 ALSExtras） =====

protected:
	UPROPERTY(VisibleDefaultsOnly, BlueprintReadOnly, Category = "Als Character")
	TObjectPtr<UAlsCameraComponent> Camera;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputMappingContext> InputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookMouseAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> LookAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> MoveAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SprintAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> WalkAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> CrouchAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> JumpAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> AimAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RagdollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RollAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> RotationModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> ViewModeAction;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings|Als Character", Meta = (DisplayThumbnail = false))
	TObjectPtr<UInputAction> SwitchShoulderAction;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookUpMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character", Meta = (ClampMin = 0, ForceUnits = "x"))
	float LookRightMouseSensitivity{1.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookUpRate{90.0f};

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings|Als Character", Meta = (ClampMin = 0, ForceUnits = "deg/s"))
	float LookRightRate{240.0f};

public:
	virtual void NotifyControllerChanged() override;

	// Camera

protected:
	virtual void CalcCamera(float DeltaTime, FMinimalViewInfo& ViewInfo) override;

	// Input

protected:
	virtual void SetupPlayerInputComponent(UInputComponent* Input) override;

	virtual void Input_OnLookMouse(const FInputActionValue& ActionValue);
	virtual void Input_OnLook(const FInputActionValue& ActionValue);
	virtual void Input_OnMove(const FInputActionValue& ActionValue);
	virtual void Input_OnSprint(const FInputActionValue& ActionValue);
	virtual void Input_OnWalk();
	virtual void Input_OnCrouch();
	virtual void Input_OnJump(const FInputActionValue& ActionValue);
	virtual void Input_OnAim(const FInputActionValue& ActionValue);
	virtual void Input_OnRagdoll();
	virtual void Input_OnRoll();
	virtual void Input_OnRotationMode();
	virtual void Input_OnViewMode();
	virtual void Input_OnSwitchShoulder();

private:
	/** 从 PlayerState/PlayerController/GameInstance 恢复角色选择并应用外观 */
	void RestoreCharacterSelection();

	// ===== 耐力系统 =====

	/** 耐力下降时施加回复延迟 GE（覆盖奔跑/翻滚/技能所有消耗来源） */
	void OnStaminaChanged(const FOnAttributeChangeData& AttributeChangeData);

	/** 服务端每帧轮询 ALS Gait/LocomotionAction，维护 PP.Status.Sprinting loose tag */
	void UpdateStaminaStateTags();

	/** 施加常驻的耐力回复/奔跑消耗 GE（Infinite 周期型，由 OngoingTagRequirements 开关） */
	void ApplyStaminaPassiveEffects();

	/** 服务端检测 ALS 进入翻滚瞬间，扣除翻滚体力 */
	void UpdateRollStaminaCost();

	/** 通过 StaminaCostEffect 扣除指定体力（SetByCaller 传 -Amount） */
	void ApplyStaminaCost(float Amount);

	/**
	 * 耐力回复 GE（Infinite + Period 0.1s，Modifier = StaminaRegenRate × 0.1）。
	 * OngoingTagRequirements 禁止 PPTags.Status.Sprinting / PPTags.Status.StaminaRegenBlocked，
	 * 实现"奔跑中不回体、消耗后 0.8s 内不回体"，无需 C++ 手写计时器。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	/**
	 * 奔跑消耗 GE（Infinite + Period 0.1s，Modifier = -0.8）。
	 * OngoingTagRequirements 要求 PPTags.Status.Sprinting，只有真的在冲刺时才跳。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaSprintDrainEffect;

	/**
	 * 耐力回复延迟 GE（Duration 型，固定 0.8s，GrantedTag = PPTags.Status.StaminaRegenBlocked）。
	 * 任何一次耐力下降都会刷新它，从而抑制常驻的 GE_StaminaRegen。
	 * 留空则耐力回复没有延迟窗口（仅用于分批调试）。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffect;

	/** 翻滚体力消耗值（体力不足时不触发翻滚） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina", Meta = (ClampMin = 0))
	float RollStaminaCost{20.f};

	/**
	 * 翻滚的体力扣除 GE（Instant，SetByCaller = PPTags.SetByCaller.StaminaCost）。
	 * 和技能共用同一个资产（GE_Cost_Stamina）。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina")
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

	/** 上一帧是否处于翻滚状态（用于检测翻滚起始瞬间） */
	bool bWasRolling = false;
};
