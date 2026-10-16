// NPC对话系统 — 对话请求接口
// 解耦 NPC(Quest模块) 对 PlayerController(主玩法模块) 的反向依赖：
// NPC 客户端交互时通过本接口请求开始对话，由 PlayerController 实现并转发到 Server RPC。

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Data/PP_DialogueData.h"
#include "PP_DialogueRequester.generated.h"

class APP_NPCActor;

UINTERFACE(BlueprintType)
class UPP_DialogueRequester : public UInterface
{
	GENERATED_BODY()
};

/**
 * IPP_DialogueRequester — 对话请求能力接口
 * 实现方：APP_PlayerController（转发到自身的 Server/Client RPC）
 * 调用方：APP_NPCActor（Interact 客户端分支 / StartDialogue 服务器分支）
 */
class PERSONALPROJECTQUEST_API IPP_DialogueRequester
{
	GENERATED_BODY()

public:
	/**
	 * 请求与指定 NPC 开始对话（客户端调用）
	 * 实现方负责：距离验证（服务端）→ NPC->StartDialogue
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Dialogue")
	void RequestStartDialogue(APP_NPCActor* NPC);

	/**
	 * 服务器权威：让玩家客户端打开对话 UI（服务器调用）
	 * 实现方负责：转发 Client_OpenDialogueUI RPC
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Dialogue")
	void OpenDialogueUI(APP_NPCActor* NPC, FName DialogueID);

	/**
	 * 上报对话结束动作（UI 侧调用）
	 * 实现方负责：内部转换为 APP_NPCActor 后转发到 Server_ConfirmDialogueEndAction RPC
	 * NPC 参数用 UObject*：UI 模块不依赖 Quest 模块具体类，仅持有对话发起方的对象引用
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Dialogue")
	void ConfirmDialogueEndAction(UObject* NPC, FName DialogueID, EPP_DialogueEndAction Action);

	/**
	 * 通知对话推进到新台词（UI 侧调用）
	 * 实现方负责：广播自身 OnDialogueLineAdvanced 委托
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Dialogue")
	void NotifyDialogueLineAdvanced(int32 NewLineIndex);

	/**
	 * 通知对话结束（UI 侧调用）
	 * 实现方负责：广播自身 OnDialogueEnded 委托
	 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "PP|Dialogue")
	void NotifyDialogueEnded(FName DialogueID, FName RelatedQuestID);
};
