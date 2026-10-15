// NPC对话系统 — NPC Actor
// 世界中可交互的 NPC，承载对话表、可选商店组件，由服务器权威触发对话与功能动作

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Characters/PP_CharacterData.h"
#include "Interaction/PP_InteractableInterface.h"
#include "PP_NPCActor.generated.h"

class UStaticMeshComponent;
class USkeletalMeshComponent;
class USphereComponent;
class UDataTable;
class UPP_ShopComponent;
class APP_PlayerController;

/**
 * NPC Actor — 对话/商店交互入口
 *
 * 职责：
 *   1. 在世界中作为可交互对象存在（带 InteractionRange 球形检测体）
 *   2. 持有 DialogueTable + DefaultDialogueID，提供对话起点
 *   3. 可选挂载 UPP_ShopComponent，使本 NPC 成为商人
 *   4. 服务器权威 StartDialogue：通过玩家 PC 的 ClientRPC 通知客户端打开对话 UI
 *   5. 提供 ResolveStartDialogueID 钩子，Task5 任务系统将根据玩家任务状态切换不同对话
 *
 * 网络模型：
 *   - Actor 本身需要 Replicates（默认开启）
 *   - StartDialogue / Interact 由服务器调用；客户端通过 PlayerController::Server_RequestStartDialogue
 *     发起请求，服务端再回调本 Actor。
 */
UCLASS()
class PERSONALPROJECT_API APP_NPCActor : public AActor, public IPP_Interactable
{
	GENERATED_BODY()

public:
	APP_NPCActor();

	// ===== 生命周期 =====

	virtual void BeginPlay() override;
	virtual void OnConstruction(const FTransform& Transform) override;

	// ===== 配置属性 =====

	/** NPC 唯一标识符（Task5 任务系统按 NPCID 关联任务接取/提交点） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FName NPCID;

	/** NPC 显示名（对话 UI 中默认作为说话者名） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC")
	FText NPCName;

	/** 对话数据表（FPP_DialogueData 行结构） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Dialogue")
	TObjectPtr<UDataTable> DialogueTable;

	/** 默认起始对话 ID（当 ResolveStartDialogueID 没有匹配到条件分支时使用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Dialogue")
	FName DefaultDialogueID;

	// ===== 交互入口 =====

	/**
	 * 蓝图/输入侧通用交互入口（任意端可调用，内部自动转发到 ServerRPC）
	 * 由 PlayerController::Interact 在 trace 命中本 NPC 后调用
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC")
	virtual void Interact(APlayerController* Player) override;

	/**
	 * 根据玩家任务状态解析起始对话 ID
	 * 目前简化为返回 DefaultDialogueID；Task5 任务系统接入后将比较 RequireQuestActiveID /
	 * RequireQuestCompletedID 选择最优对话行。
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC|Dialogue")
	FName ResolveStartDialogueID(APP_PlayerController* Player) const;

	/**
	 * 服务器权威：启动一段对话
	 * 内部解析起始 DialogueID → 通过玩家 PC 的 Client RPC 打开对话 UI
	 * @param Player 发起交互的玩家控制器（必须非空）
	 */
	UFUNCTION(BlueprintCallable, Category = "NPC|Dialogue")
	void StartDialogue(APlayerController* Player);

	/** 获取 NPC 挂载的商店组件（可能为空） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC|Shop")
	UPP_ShopComponent* GetShopComponent() const { return ShopComponent; }

	/** 获取对话表（供 PC 端 UI 查询对话行） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "NPC|Dialogue")
	UDataTable* GetDialogueTable() const { return DialogueTable; }

protected:
	// ===== 组件 =====

	/** 根组件，负责碰撞 + 静态外观（无骨骼动画时作为 fallback） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<UStaticMeshComponent> MeshComponent;

	/** NPC 人物外观 — 骨骼网格体（支持角色骨架 + 动画蓝图） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<USkeletalMeshComponent> BodyMesh;

	/**
	 * 角色外观配置（骨骼网格 + 动画蓝图）
	 * 配置后自动在 BeginPlay 应用，并隐藏静态 MeshComponent；
	 * 留空则退化为纯静态网格 NPC（向后兼容）
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "NPC|Appearance")
	FCharacterDataRow NPCAppearance;

	/** 应用角色外观到 BodyMesh，同时隐藏静态网格 */
	UFUNCTION(BlueprintCallable, Category = "NPC|Appearance")
	void ApplyNPCAppearance();

	/** 交互范围球形检测体（玩家进入后可触发交互提示，距离验证服务器侧也会复用其半径） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<USphereComponent> InteractionRange;

	/** 可选商店组件，挂载后此 NPC 即为商人；通过对话 EndAction=OpenShop 打开 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "NPC|Components")
	TObjectPtr<UPP_ShopComponent> ShopComponent;
};
