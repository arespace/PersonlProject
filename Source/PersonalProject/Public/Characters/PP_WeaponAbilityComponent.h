// 武器技能组件 — 按装备槽授予/移除武器 GA（仅服务器）

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Data/PP_ItemTypes.h"
#include "GameplayAbilitySpec.h"
#include "PP_WeaponAbilityComponent.generated.h"

class UAbilitySystemComponent;

/**
 * 武器技能组件。
 *
 * 职责：把"当前装备的武器"映射成 ASC 上的 GA——
 *   - 装备变化：移除该槽上一次授予的 Spec，再按 DT_Items.WeaponConfig.WeaponAbilityClass 授予新的；
 *   - 出生 / 复活 / 重连：按 PlayerState 上已复制的装备恢复（已授予过则跳过，重复调用结果一致）。
 *
 * 服务器权威：授予/移除只在权威端执行，客户端调用直接返回。
 * 不参与网络复制：WeaponAbilityHandles 只是服务器本地的清理账本。
 *
 * 归属 PersonalProject 模块（不是 Core）：恢复时要读 APP_PlayerState 上的 UPP_InventoryComponent
 * （PersonalProjectInventory 模块），放进 Core 会造成反向依赖。
 * ASC 通过 IAbilitySystemInterface 获取，不直接依赖具体角色类。
 */
UCLASS(ClassGroup = (PP), meta = (BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_WeaponAbilityComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_WeaponAbilityComponent();

	/** 唯一的刷新入口：按 PlayerState 上已复制的装备同步 GA（已授予同一件则跳过，重复调用结果一致；仅服务器） */
	void RefreshAbilityFromInventory();

private:
	/** 授予/替换某武器槽对应的 GA（仅服务器；会先移除该槽上一次授予的 GA） */
	void GrantWeaponAbility(EPP_EquipSlot Slot, FName NewItemID);

	/** 通过 IAbilitySystemInterface 取拥有者的 ASC（ASC 在 PlayerState 上） */
	UAbilitySystemComponent* GetOwnerAbilitySystemComponent() const;

	/** 每个武器槽位当前激活的 AbilitySpecHandle（卸下/替换时用于精确移除；仅服务器维护） */
	TMap<EPP_EquipSlot, FGameplayAbilitySpecHandle> WeaponAbilityHandles;

	/** 每个武器槽位上一次同步的 ItemID（用于判断"装备没变"从而跳过重复授予；仅服务器维护） */
	TMap<EPP_EquipSlot, FName> GrantedItemIDs;
};
