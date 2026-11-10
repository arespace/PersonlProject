
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

	/** 耐力下降时施加回复延迟 GE（覆盖奔跑/翻滚/技能所有消耗来源） */
	void OnStaminaChanged(const FOnAttributeChangeData& AttributeChangeData);

	/** 服务端每帧轮询 ALS Gait/LocomotionAction，维护 PP.Status.Sprinting loose tag */
	void UpdateStaminaStateTags();

	/** 施加常驻的耐力回复/奔跑消耗 GE（Infinite 周期型，由 OngoingTagRequirements 开关） */
	void ApplyStaminaPassiveEffects();

	/** 服务端检测 ALS 进入翻滚瞬间，扣除翻滚体力 */
	void UpdateRollStaminaCost();

	/** 服务端同步 ALS 翻滚状态到 Status.Invulnerable（i-frames 无敌窗口），跟随 Rolling 起止 */
	void UpdateRollInvulnerability();

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

	// ===== 武器外观（自带烘焙武器隐藏 / 装备武器网格显示） =====

	/** 切换自带（烘焙在骨骼网格上的）武器是否可见：材质槽换成隐形材质 */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void SetBuiltInWeaponVisible(bool bVisible);

	/** 把 WeaponMesh 换成 ItemID 对应的武器网格（读 DT_Items 的 WeaponConfig.Visual） */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void SetWeaponMesh(FName ItemID);

	/** 清空 WeaponMesh（空手） */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void ClearWeaponMesh();

	/** 自带武器占用的材质槽索引（如 Kwang 的 17） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual", meta = (AllowPrivateAccess = "true"))
	TArray<int32> BuiltInWeaponMaterialSlots;

	/** 隐藏自带武器时使用的全透明材质 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UMaterialInterface> HiddenWeaponMaterial;

	/** 装备武器网格挂载的骨骼/socket 名；仅当武器行未配置 AttachSocket 时兜底 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual", meta = (AllowPrivateAccess = "true"))
	FName WeaponAttachSocketName{TEXTVIEW("VB_Kwang_weapon")};

	/** BeginPlay 时是否立即隐藏自带武器（默认空手） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual", meta = (AllowPrivateAccess = "true"))
	bool bHideBuiltInWeaponOnBeginPlay{true};

	/** 装备武器的网格容器（默认无网格，挂在 WeaponAttachSocketName 上） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Weapon|Visual", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	/** 客户端窗口执行 PPAddItem 时转发到服务器 */
	UFUNCTION(Server, Reliable)
	void ServerPPAddItem(FName ItemID, int32 Count);

	/** 服务器实际执行的加物品逻辑 */
	void GrantItemToInventory(FName ItemID, int32 Count);

private:
	/** 按 PlayerState 上已复制的武器槽恢复外观与（服务端）武器 GA，用于出生 / 复活 / 重连 */
	void RestoreEquippedWeapon();

	/** 授予/替换某武器槽对应的 GA（仅服务器调用；会先移除该槽上一次授予的 GA） */
	void GrantWeaponAbility(EPP_EquipSlot Slot, FName NewItemID);

	/** 每个武器槽位当前激活的 AbilitySpecHandle（卸下/替换时用于精确移除） */
	TMap<EPP_EquipSlot, FGameplayAbilitySpecHandle> WeaponAbilityHandles;

	/** 自带武器材质槽的原始材质缓存（与 BuiltInWeaponMaterialSlots 一一对应） */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> CachedBuiltInWeaponMaterials;
};
