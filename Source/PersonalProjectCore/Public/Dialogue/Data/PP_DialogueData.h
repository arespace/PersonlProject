// NPC对话系统 — 对话数据结构定义
// 包含对话结束动作枚举与对话行数据表行结构（FPP_DialogueData）

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PP_DialogueData.generated.h"

/**
 * 对话结束时的动作类型
 * 控制对话最后一行后弹出的功能选项面板内容
 */
UENUM(BlueprintType)
enum class EPP_DialogueEndAction : uint8
{
	/** 仅结束对话，不显示任何后续选项 */
	None				UMETA(DisplayName = "None"),
	/** 显示「接受任务」按钮（具体接取逻辑由 Task5 任务系统完善） */
	OfferQuest			UMETA(DisplayName = "OfferQuest"),
	/** 显示「提交任务」按钮（具体提交逻辑由 Task5 任务系统完善） */
	SubmitQuest			UMETA(DisplayName = "SubmitQuest"),
	/** 打开 NPC 挂载的商店面板 */
	OpenShop			UMETA(DisplayName = "OpenShop"),
	/** 链式跳转到 NextDialogueID 指定的下一段对话 */
	ChainNextDialogue	UMETA(DisplayName = "ChainNextDialogue")
};

/**
 * 对话数据行 — DataTable 行结构
 *
 * 单段线性对话配置：
 *   - DialogueID：行键（唯一）
 *   - SpeakerName：UI上显示的说话人名（一般为 NPC 名）
 *   - Lines：按顺序播放的台词数组，UI 通过「下一句」按钮逐条推进
 *   - EndAction：最后一行后弹出的功能选项类型
 *   - RelatedQuestID：与 OfferQuest/SubmitQuest 配合，标识关联的任务ID（Task5 使用）
 *   - NextDialogueID：与 ChainNextDialogue 配合，标识下一段对话的行键
 *   - RequireQuestActiveID / RequireQuestCompletedID：基于玩家任务状态切换不同对话的判定条件
 *     （Task5 任务系统接入后，APP_NPCActor::ResolveStartDialogueID 会根据这两个字段挑选合适的对话行）
 */
USTRUCT(BlueprintType)
struct FPP_DialogueData : public FTableRowBase
{
	GENERATED_BODY()

	FPP_DialogueData()
		: EndAction(EPP_DialogueEndAction::None)
	{}

	/** 对话唯一标识符（等同于 DataTable 行键，方便代码内交叉引用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FName DialogueID;

	/** 说话者显示名（一般使用 NPC 显示名，亦可通过本字段覆盖） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	FText SpeakerName;

	/** 顺序播放的台词数组，按下「下一句」逐条推进 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue", meta = (MultiLine = true))
	TArray<FText> Lines;

	/** 对话最后一行后弹出的功能选项类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue")
	EPP_DialogueEndAction EndAction;

	/** OfferQuest / SubmitQuest 关联的任务ID（Task5 接入） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Quest")
	FName RelatedQuestID;

	/** ChainNextDialogue 跳转的下一段对话 DialogueID */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Chain")
	FName NextDialogueID;

	/** 仅当玩家正在进行此 QuestID 时使用本对话（Task5 接入） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Condition")
	FName RequireQuestActiveID;

	/** 仅当玩家已完成此 QuestID 时使用本对话（Task5 接入） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialogue|Condition")
	FName RequireQuestCompletedID;
};
