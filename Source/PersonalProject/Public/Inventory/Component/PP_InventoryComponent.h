// 库存系统 — 库存与装备管理组件
// 服务器权威的物品/货币/装备管理，通过网络复制同步至客户端

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Inventory/Data/PP_ItemData.h"
#include "Inventory/Data/PP_ItemTypes.h"
#include "PP_InventoryComponent.generated.h"

class UDataTable;
class UAbilitySystemComponent;
class UGameplayEffect;
class APP_PlayerState;

/** 库存内容变化广播：物品ID + 当前总数（0表示已移除） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventoryChanged, FName, ItemID, int32, NewCount);

/** 装备槽位变化广播：槽位 + 当前装备的物品ID（NAME_None 表示已脱下） */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEquipmentChanged, EPP_EquipSlot, Slot, FName, ItemID);

/** 货币变化广播：当前金币数 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);

/** 物品使用事件广播：消耗品被成功使用 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, FName, ItemID);

/**
 * 库存与装备管理组件
 *
 * 职责：
 *   1. 维护背包物品列表（FPP_ItemInstance），支持堆叠、增删查询
 *   2. 维护10个装备槽位，通过 GAS GameplayEffect 提供属性加成
 *   3. 维护货币（金币）数量
 *   4. 通过 ReplicatedUsing 在服务器与客户端间同步状态
 *   5. 通过委托广播变化事件，供 UI 与任务系统监听
 *
 * 网络模型：所有写操作均在服务器执行（Server RPC 或本地服务端调用），
 *          客户端通过 OnRep_* 回调更新 UI。
 *
 * 使用约定：通常挂载在 APP_PlayerState 上，确保跨地图传输时仍可保留库存数据。
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PERSONALPROJECT_API UPP_InventoryComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UPP_InventoryComponent();

	//~ Begin UActorComponent Interface
	virtual void BeginPlay() override;
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	//~ End UActorComponent Interface

	// ===== 物品基础接口（服务器权威调用） =====

	/**
	 * 向库存中添加物品
	 * 优先填充已存在的同ID堆叠槽，若达到 MaxStackCount 则新建条目
	 * @param ItemID 物品ID（必须存在于 ItemDataTable 中）
	 * @param Count  添加数量（必须 > 0）
	 * @return 是否成功添加（任何一件成功即返回 true）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool AddItem(FName ItemID, int32 Count);

	/**
	 * 从库存中移除物品
	 * 按数组顺序依次扣减堆叠槽，全部清零后移除条目
	 * @param ItemID 物品ID
	 * @param Count  移除数量
	 * @return 是否成功（库存不足时返回 false 且不扣减）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory")
	bool RemoveItem(FName ItemID, int32 Count);

	/**
	 * 查询库存中某物品的总数
	 * @param ItemID 物品ID
	 * @return 该物品在所有堆叠槽中的累计数量
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	int32 GetItemCount(FName ItemID) const;

	/**
	 * 判断库存中是否存在足够数量的某物品
	 * @param ItemID 物品ID
	 * @param Count  需要的最低数量（默认1）
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	bool HasItem(FName ItemID, int32 Count = 1) const;

	/** 获取背包当前所有物品实例的副本，UI 渲染用 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory")
	TArray<FPP_ItemInstance> GetItems() const { return Items; }

	// ===== 装备接口（服务器权威，客户端通过 RPC 触发） =====

	/**
	 * 客户端请求穿戴指定物品
	 * 服务器执行：从背包扣减 → 替换前一件装备（如有）→ 通过 ASC 施加 GE → 写入 EquippedItems
	 * @param ItemID 要穿戴的物品ID（必须为 Equipment 类型且在背包中）
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|Equipment")
	void Server_EquipItem(FName ItemID);

	/**
	 * 客户端请求脱下指定槽位的装备
	 * 服务器执行：移除 GE → 装备回到背包 → 从 EquippedItems 移除
	 * @param Slot 要脱下的装备槽位
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|Equipment")
	void Server_UnequipItem(EPP_EquipSlot Slot);

	/** 获取指定槽位当前装备的物品ID（NAME_None 表示空） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Equipment")
	FName GetEquippedItemID(EPP_EquipSlot Slot) const;

	/** 获取所有已装备物品的副本，UI 渲染用 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Equipment")
	TArray<FPP_EquippedItem> GetEquippedItems() const { return EquippedItems; }

	// ===== 消耗品接口 =====

	/**
	 * 客户端请求使用消耗品
	 * 服务器验证 → 扣减 → 施加 ConsumeEffectClass GE → 广播事件
	 */
	UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Inventory|Consumable")
	void Server_UseConsumable(FName ItemID);

	/** 获取指定槽位当前已装备的物品（用于属性对比），无则返回空 FPP_EquippedItem */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Equipment")
	FPP_EquippedItem GetEquippedItemInSlot(EPP_EquipSlot Slot) const;

	// ===== 货币接口 =====

	/** 获取当前金币数量 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "Inventory|Gold")
	int32 GetGold() const { return Gold; }

	/**
	 * 扣减金币（不会扣到负数）
	 * @param Amount 扣减数量（必须 > 0）
	 * @return 余额是否足够并成功扣减
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Gold")
	bool SpendGold(int32 Amount);

	/**
	 * 增加金币
	 * @param Amount 增加数量（必须 > 0）
	 */
	UFUNCTION(BlueprintCallable, Category = "Inventory|Gold")
	void AddGold(int32 Amount);

	// ===== 数据表查询辅助 =====

	/** 根据 ItemID 从 DataTable 中查找物品配置 */
	const FPP_ItemInfo* FindItemInfo(FName ItemID) const;

	// ===== 委托 =====

	/** 库存条目变化时广播（添加/移除/堆叠数量变化） */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnInventoryChanged OnInventoryChanged;

	/** 装备槽位变化时广播（穿戴/脱下） */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnEquipmentChanged OnEquipmentChanged;

	/** 金币变化时广播 */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnGoldChanged OnGoldChanged;

	/** 消耗品使用事件广播（任务系统可监听） */
	UPROPERTY(BlueprintAssignable, Category = "Inventory|Events")
	FOnItemUsed OnItemUsed;

protected:
	// ===== OnRep 回调 =====

	UFUNCTION()
	void OnRep_Items(const TArray<FPP_ItemInstance>& OldItems);

	UFUNCTION()
	void OnRep_Equipped(const TArray<FPP_EquippedItem>& OldEquipped);

	UFUNCTION()
	void OnRep_Gold(int32 OldGold);

	// ===== 复制属性 =====

	/** 物品实例列表（服务器权威） */
	UPROPERTY(ReplicatedUsing = OnRep_Items, BlueprintReadOnly, Category = "Inventory")
	TArray<FPP_ItemInstance> Items;

	/** 已装备物品列表（服务器权威） */
	UPROPERTY(ReplicatedUsing = OnRep_Equipped, BlueprintReadOnly, Category = "Inventory")
	TArray<FPP_EquippedItem> EquippedItems;

	/** 金币数量（服务器权威） */
	UPROPERTY(ReplicatedUsing = OnRep_Gold, BlueprintReadOnly, Category = "Inventory")
	int32 Gold;

	// ===== 配置 =====

	/** 物品配置数据表（FPP_ItemInfo 行结构） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config")
	TObjectPtr<UDataTable> ItemDataTable;

	/** 背包最大条目数（堆叠槽数量），0 表示无限 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Inventory|Config", meta = (ClampMin = "0"))
	int32 MaxInventorySlots = 50;

private:
	// ===== 服务器内部实现 =====

	/** 服务端实际执行的穿戴逻辑 */
	void ApplyEquip(const FPP_ItemInfo& ItemInfo);

	/** 服务端实际执行的脱下逻辑，返回脱下物品的ID */
	FName ApplyUnequip(EPP_EquipSlot Slot);

	/** 服务端内部：使用消耗品逻辑（移除物品 + 施加 GE） */
	void ApplyConsumable(const FPP_ItemInfo& ItemInfo);

	/** 获取拥有者的 ASC（用于施加/移除 GE） */
	UAbilitySystemComponent* GetOwningAbilitySystemComponent() const;
};
