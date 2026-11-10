// 武器外观组件 — 装备武器网格 + 自带（烘焙）武器隐藏

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PP_WeaponVisualComponent.generated.h"

class UStaticMeshComponent;
class UMaterialInterface;
class USkeletalMeshComponent;

/**
 * 武器外观组件。
 *
 * 职责：把"当前装备的武器"映射成视觉表现——
 *   - 有武器：显示 DT_Items.WeaponConfig.Visual 指定的静态网格，挂到 socket / 右手骨上；
 *   - 空手：清空网格，并把角色网格上的自带武器材质槽还原。
 *
 * 归属 PersonalProject 模块（不是 Core）：恢复外观时要读 APP_PlayerState 上的
 * UPP_InventoryComponent（PersonalProjectInventory 模块），放进 Core 会造成反向依赖。
 *
 * 不负责 GA 授予（仍在 APP_PlayerCharacter 上），也不直接监听装备复制；
 * 由角色在 PossessedBy / OnRep_PlayerState 以及 OnEquipmentChanged 里驱动。
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_WeaponVisualComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_WeaponVisualComponent();

	virtual void BeginPlay() override;

	/** 装备栏变化入口：空手 → 清网格并显示自带武器；有武器 → 隐藏自带武器并显示对应网格 */
	void ApplyEquippedWeapon(FName ItemID);

	/** 按 PlayerState 上已复制的装备恢复外观（出生 / 复活 / 重连 / 客户端收到复制都能调，结果一致） */
	void RefreshVisualFromInventory();

	/** 把武器网格换成 ItemID 对应的静态网格（读 DT_Items 的 WeaponConfig.Visual） */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void SetWeaponMesh(FName ItemID);

	/** 清空武器网格（空手） */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void ClearWeaponMesh();

	/** 切换自带（烘焙在骨骼网格上的）武器是否可见：材质槽换成隐形材质 */
	UFUNCTION(BlueprintCallable, Category = "PP|Weapon|Visual")
	void SetBuiltInWeaponVisible(bool bVisible);

	/** 装备武器的网格容器（组件子对象，默认无网格） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "PP|Weapon|Visual")
	TObjectPtr<UStaticMeshComponent> WeaponMesh;

	// ===== 配置（原在 APP_PlayerCharacter 上，迁移后在本组件上覆盖） =====

	/** 装备武器网格挂载的骨骼/socket 名；仅当武器行未配置 AttachSocket 时兜底 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual")
	FName WeaponAttachSocketName{TEXTVIEW("VB_Kwang_weapon")};

	/** 自带武器占用的材质槽索引（如 Kwang 的 17）；留空则按材质名自动识别 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual")
	TArray<int32> BuiltInWeaponMaterialSlots;

	/** 隐藏自带武器时使用的全透明材质 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual")
	TObjectPtr<UMaterialInterface> HiddenWeaponMaterial;

	/** BeginPlay 时是否立即隐藏自带武器（默认空手） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "PP|Weapon|Visual")
	bool bHideBuiltInWeaponOnBeginPlay{true};

private:
	/** BeginPlay 初始化：挂 socket、识别材质槽、缓存原始材质、按配置隐藏自带武器 */
	void InitializeVisual();

	/** 把角色上已废弃的同名配置灌进组件（只灌与默认值不同的项，打一条汇总 Warning） */
	void MigrateDeprecatedSettings();

	/** 把武器网格挂到 socket 上，socket 不存在时按 Paragon 通用挂点顺序回退 */
	static void AttachToSocket(USkeletalMeshComponent* SkeletalMesh, UStaticMeshComponent* WeaponMeshComponent,
		FName SocketName, const FVector& RelativeLocation, const FRotator& RelativeRotation, const FVector& RelativeScale);

	/** 按材质名自动识别自带武器材质槽（网格可能在 BeginPlay 之后才被角色外观逻辑替换，需惰性调用） */
	static void AutoDetectMaterialSlots(USkeletalMeshComponent* SkeletalMesh, TArray<int32>& OutSlots);

	/** 从网格资产取原始材质（不依赖运行时缓存，网格被替换后也能正确还原） */
	static UMaterialInterface* GetSkeletalMeshAssetMaterial(USkeletalMeshComponent* SkeletalMesh, int32 SlotIndex);

	/** 自带武器材质槽的原始材质缓存（与 BuiltInWeaponMaterialSlots 一一对应） */
	UPROPERTY()
	TArray<TObjectPtr<UMaterialInterface>> CachedBuiltInWeaponMaterials;

	/** 旧配置是否已完成迁移（避免重复灌值与重复告警） */
	bool bDeprecatedSettingsMigrated{false};
};
