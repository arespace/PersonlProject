
#pragma once

#include "CoreMinimal.h"
#include "Characters/PP_BaseCharacter.h"
#include "Characters/PP_CharacterData.h"
#include "Characters/PP_LockOnComponent.h"
#include "Data/PP_ItemTypes.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "Interfaces/PP_EquipmentUserInterface.h"
#include "PP_PlayerCharacter.generated.h"

struct FInputActionValue;
class UAlsCameraComponent;
class UInputMappingContext;
class UInputAction;
class APP_SoulPickupActor;
class UStaticMesh;
class UStaticMeshComponent;
class UMaterialInterface;
class UGameplayEffect;
class UAnimMontage;
class USkeletalMesh;
class UPP_StaminaComponent;
class UPP_WeaponAbilityComponent;
class UPP_WeaponVisualComponent;

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

	// ===== 调试命令（控制台：PPAddItem Longsword 1，需在服务器窗口执行） =====

	/** 给本机玩家背包添加物品，用于测试装备/武器流程 */
	UFUNCTION(Exec)
	void PPAddItem(FName ItemID, int32 Count = 1);

	// ===== 锁定系统（仅本地玩家生效：目标选择/相机朝向/角色朝向；服务端依靠 ALS 视角同步） =====

	/** 当前锁定的敌人（无锁定时为 nullptr） */
	AActor* GetLockOnTarget() const { return LockOnComponent ? LockOnComponent->GetLockOnTarget() : nullptr; }

	/** 锁定目标的世界坐标焦点（胸口高度，供 HUD 绘制标记） */
	FVector GetLockOnFocusLocation() const;

	bool IsLockedOn() const { return LockOnComponent && LockOnComponent->IsLockedOn(); }

	// ===== ALS 相机与输入（移植自 AAlsCharacterExample，避免依赖 demo 模块 ALSExtras） =====

protected:
	virtual void BeginPlay() override;

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

	/**
	 * 开关 ALS 移动 IMC（priority 0）。
	 * Modal / FrontEnd 下关闭，避免 GameAndUI 时角色仍能走动、技能仍能被触发。
	 */
	void SetLocomotionInputEnabled(bool bEnabled);

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

	/** 装备长剑：从背包穿戴 Longsword（原蓝图 F 键原生事件，现由 IA_EquipLongsword 驱动） */
	void EquipLongsword();

	/** ALS 翻滚蒙太奇选择：按当前骨骼网格查表，未命中时回退到 ALS 默认蒙太奇 */
	virtual UAnimMontage* SelectRollMontage_Implementation() override;

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

	/** 服务端同步 ALS 翻滚状态到 Status.Invulnerable（i-frames 无敌窗口），跟随 Rolling 起止 */
	void UpdateRollInvulnerability();

	/**
	 * 把旧的"角色级"耐力配置迁移到 StaminaComponent（过渡保护）。
	 * PossessedBy / OnRep_PlayerState / BeginPlay 都会调用，内部只执行一次，只覆盖与组件默认值不同的项。
	 */
	void TransferDeprecatedStaminaSettings();

	/** 耐力组件：消耗 / 恢复 / 恢复延迟 / 冲刺与翻滚扣费都在组件里 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Stamina", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_StaminaComponent> StaminaComponent;

	// ===== 耐力系统：已迁移到 UPP_StaminaComponent 的旧属性（过渡期保留，确认蓝图后单独提交删除） =====

	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina",
		Meta = (DeprecatedProperty, DeprecationMessage = "已迁移到 StaminaComponent 的 StaminaRegenEffect，请在组件上配置"))
	TSubclassOf<UGameplayEffect> StaminaRegenEffect;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina",
		Meta = (DeprecatedProperty, DeprecationMessage = "已迁移到 StaminaComponent 的 StaminaSprintDrainEffect，请在组件上配置"))
	TSubclassOf<UGameplayEffect> StaminaSprintDrainEffect;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina",
		Meta = (DeprecatedProperty, DeprecationMessage = "已迁移到 StaminaComponent 的 StaminaRegenDelayEffect，请在组件上配置"))
	TSubclassOf<UGameplayEffect> StaminaRegenDelayEffect;

	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina",
		Meta = (DeprecatedProperty, DeprecationMessage = "已迁移到 StaminaComponent 的 RollStaminaCost，请在组件上配置"))
	float RollStaminaCost{20.f};

	UPROPERTY(EditDefaultsOnly, Category = "PP|Stamina",
		Meta = (DeprecatedProperty, DeprecationMessage = "已迁移到 StaminaComponent 的 StaminaCostEffect，请在组件上配置"))
	TSubclassOf<UGameplayEffect> StaminaCostEffect;

	/** 旧属性是否已迁移到组件（避免重复灌值与重复告警） */
	bool bDeprecatedStaminaTransferred{false};

	// ===== 翻滚蒙太奇 =====

	/**
	 * 按骨骼网格指定翻滚蒙太奇：角色换皮/换模型时自动使用对应动作。
	 * 未命中时回退到 ALS 的 Settings->Rolling.Montage，留空即为纯 ALS 行为。
	 */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Roll")
	TMap<TObjectPtr<USkeletalMesh>, TObjectPtr<UAnimMontage>> RollMontageByMesh;

	// ===== 锁定系统 =====

	/** 锁定组件：目标选择、相机朝向、解除锁定逻辑都在组件里 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|LockOn", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_LockOnComponent> LockOnComponent;

	/** 锁定/解锁输入动作（IA_LockOn，默认映射数字键 6；在玩家蓝图中指定资产） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|LockOn")
	TObjectPtr<UInputAction> LockOnAction;

	/** 装备长剑输入动作（IA_EquipLongsword；原为蓝图 EventGraph 里的 F 键原生事件，现统一走 IA） */
	UPROPERTY(EditDefaultsOnly, Category = "PP|Input|Debug")
	TObjectPtr<UInputAction> EquipLongswordAction;

	/** ALS 移动 IMC 当前是否激活，避免重复 Add/Remove，并让重生时保持上下文一致 */
	bool bLocomotionInputEnabled = true;

	// ===== 武器装备接口（IPP_EquipmentUser 实现） =====

	/** 武器装备槽变更回调 — 从 DT_Items 的 WeaponConfig 取 GA → ASC 切换 */
	virtual void OnEquipmentChanged_Implementation(EPP_EquipSlot Slot, FName NewItemID) override;

	// ===== 武器外观（薄外壳：实现已迁移到 WeaponVisualComponent） =====

	/** 武器外观组件：装备武器网格显示 + 自带烘焙武器隐藏 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Weapon|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_WeaponVisualComponent> WeaponVisualComponent;

	/** 切换自带（烘焙在骨骼网格上的）武器是否可见：材质槽换成隐形材质 */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void SetBuiltInWeaponVisible(bool bVisible);

	/** 把武器网格换成 ItemID 对应的武器网格（读 DT_Items 的 WeaponConfig.Visual） */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void SetWeaponMesh(FName ItemID);

	/** 清空武器网格（空手） */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void ClearWeaponMesh();

	// ===== 武器外观：已迁移到 UPP_WeaponVisualComponent 的旧属性（过渡期保留，确认蓝图后单独提交删除） =====

	/** 自带武器占用的材质槽索引（如 Kwang 的 17） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual",
		Meta = (AllowPrivateAccess = "true", DeprecatedProperty,
			DeprecationMessage = "已迁移到 WeaponVisualComponent 的 BuiltInWeaponMaterialSlots，请在组件上配置"))
	TArray<int32> BuiltInWeaponMaterialSlots;

	/** 隐藏自带武器时使用的全透明材质 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual",
		Meta = (AllowPrivateAccess = "true", DeprecatedProperty,
			DeprecationMessage = "已迁移到 WeaponVisualComponent 的 HiddenWeaponMaterial，请在组件上配置"))
	TObjectPtr<UMaterialInterface> HiddenWeaponMaterial;

	/** 装备武器网格挂载的骨骼/socket 名；仅当武器行未配置 AttachSocket 时兜底 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual",
		Meta = (AllowPrivateAccess = "true", DeprecatedProperty,
			DeprecationMessage = "已迁移到 WeaponVisualComponent 的 WeaponAttachSocketName，请在组件上配置"))
	FName WeaponAttachSocketName{TEXTVIEW("VB_Kwang_weapon")};

	/** BeginPlay 时是否立即隐藏自带武器（默认空手） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual",
		Meta = (AllowPrivateAccess = "true", DeprecatedProperty,
			DeprecationMessage = "已迁移到 WeaponVisualComponent 的 bHideBuiltInWeaponOnBeginPlay，请在组件上配置"))
	bool bHideBuiltInWeaponOnBeginPlay{true};

	/** 客户端窗口执行 PPAddItem 时转发到服务器 */
	UFUNCTION(Server, Reliable)
	void ServerPPAddItem(FName ItemID, int32 Count);

	/** 服务器实际执行的加物品逻辑 */
	void GrantItemToInventory(FName ItemID, int32 Count);

private:
	/** 武器外观组件需要读取本类上已废弃的旧配置做一次性迁移 */
	friend class UPP_WeaponVisualComponent;

	/** 按 PlayerState 上已复制的武器槽恢复（服务端）武器 GA（薄外壳，实现在 WeaponAbilityComponent） */
	void RestoreEquippedWeaponAbility();

	/** 武器技能组件：装备武器 → 授予/移除 GA（仅服务器） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Weapon|Ability", Meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UPP_WeaponAbilityComponent> WeaponAbilityComponent;
};
