// Chen Tianbao uses GAS to create online battle games

#pragma once

#include "CoreMinimal.h"
#include "Characters/PP_BaseCharacter.h"
#include "Characters/PP_CharacterData.h"
#include "Data/PP_ItemTypes.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "Interaction/PP_InteractableInterface.h"
#include "PP_PlayerCharacter.generated.h"

struct FInputActionValue;
class UAlsCameraComponent;
class UInputMappingContext;
class UInputAction;
class APP_SoulPickupActor;
class UDataTable;

UCLASS()
class PERSONALPROJECT_API APP_PlayerCharacter : public APP_BaseCharacter, public IPP_EquipmentUser
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

	/** 复活点优先取 PlayerState 里篝火记录的位置，没有记录时回退到 PlayerStart */
	virtual bool GetRespawnTransform(FTransform& OutTransform) override;

public:
	/** 根据角色ID应用外观（模型+动画） */
	void ApplyCharacterAppearance(FName CharacterId);

	// ===== 锁定系统（仅本地玩家生效：目标选择/相机朝向/角色朝向；服务端依靠 ALS 视角同步） =====

	/** 当前锁定的敌人（无锁定时为 nullptr） */
	AActor* GetLockOnTarget() const { return LockOnTarget.Get(); }

	/** 锁定目标的世界坐标焦点（胸口高度，供 HUD 绘制标记） */
	FVector GetLockOnFocusLocation() const;

	bool IsLockedOn() const { return LockOnTarget.IsValid(); }

	// ===== ALS 相机与输入（移植自 AAlsCharacterExample，避免依赖 demo 模块 ALSExtras） =====

protected:
	/** 玩家死亡：基类处理死亡/复活计时，这里额外把携带的金币掉落成魂 */
	virtual void HandleDeath() override;

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
	virtual void Input_OnLockOn();

private:
	/** 从 PlayerState/PlayerController/GameInstance 恢复角色选择并应用外观 */
	void RestoreCharacterSelection();

	// ===== 死亡掉魂 =====

	/** 死亡时化为魂的金币比例（0 = 不掉落，1 = 全部携带金币） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|DeathSoul", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float DeathSoulDropRatio{1.f};

	/** 掉落魂使用的 Actor 类（可在角色蓝图中替换成带特效的 BP_SoulPickup） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|DeathSoul")
	TSubclassOf<APP_SoulPickupActor> SoulPickupClass;

	/** 上一次死亡留下、尚未拾回的魂（再次死亡时销毁，视为永久丢失） */
	TWeakObjectPtr<APP_SoulPickupActor> ActiveSoul;

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

	// ===== 锁定系统 =====

	/** 锁定/解锁按键（默认 Q，可在玩家蓝图里改） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn")
	FKey LockOnKey;

	/** 搜索敌人的最大半径（cm） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn", Meta = (ClampMin = 0))
	float LockOnSearchRadius{1200.f};

	/** 超过该距离自动解除锁定（cm，需大于搜索半径形成滞回） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn", Meta = (ClampMin = 0))
	float LockOnBreakDistance{1600.f};

	/** 目标必须位于相机前方这个半角内才会被锁定（度） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn", Meta = (ClampMin = 0, ClampMax = 180))
	float LockOnMaxScreenAngle{60.f};

	/** 相机朝向目标的插值速度（越大转得越快） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn", Meta = (ClampMin = 0))
	float LockOnCameraInterpolationSpeed{10.f};

	/** 目标焦点相对敌人 Actor 原点的高度偏移（cm，取胸口高度） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn")
	float LockOnFocusHeight{60.f};

	/** 当前锁定目标（仅本地玩家维护，不参与网络复制） */
	TWeakObjectPtr<AActor> LockOnTarget;

	/** 锁定前的 ALS 期望旋转模式，解除锁定时恢复 */
	FGameplayTag PreLockOnRotationMode;

	/** 切换锁定：已锁定则解锁，否则搜索并锁定最近的敌人 */
	void ToggleLockOn();

	/** 在搜索半径/视锥内寻找最近的可见敌人 */
	AActor* FindLockOnTarget();

	/** 锁定目标是否仍然有效（存活且未超出解除距离） */
	bool IsLockOnTargetValid(const AActor* Target) const;

	/** 本地玩家每帧驱动相机朝向目标，并校验目标有效性 */
	void UpdateLockOn(float DeltaTime);

	/** 解除锁定并恢复锁定前的旋转模式 */
	void ReleaseLockOn();

	// ===== 武器装备接口（IPP_EquipmentUser 实现） =====

	/** 武器装备槽变更回调 — 从 DT_WeaponData 查 AbilityTags → ASC 切换 GA */
	virtual void OnEquipmentChanged_Implementation(EPP_EquipmentSlotType SlotType, FName NewItemID) override;

	/** 武器数据表（DT_WeaponData，行结构 FPP_WeaponData） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Weapon")
	TObjectPtr<UDataTable> WeaponDataTable;

private:
	/** 每个武器槽位当前激活的 AbilitySpecHandle（卸下/替换时用于精确移除） */
	TMap<EPP_EquipmentSlotType, FGameplayAbilitySpecHandle> WeaponAbilityHandles;
};
