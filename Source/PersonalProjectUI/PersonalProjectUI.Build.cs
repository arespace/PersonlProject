// PersonalProjectUI 模块 — 用户界面层（HUD、UIManager、全部 Widget）
// 依赖方向：UI → 主玩法 → Quest → Inventory → Core（单向无环）
// PlayerController 的 UI 相关函数实现（PP_UICrossModule.cpp）位于本模块

using UnrealBuildTool;

public class PersonalProjectUI : ModuleRules
{
	public PersonalProjectUI(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"UMG",
			"Slate",
			"SlateCore",
			"GameplayAbilities",
			"GameplayTags",
			"EnhancedInput",
			"PersonalProjectCore",
			// 不能删：UPP_DialogueRequester 定义在 PersonalProjectQuest，对话 Widget 与 PP_UICrossModule.cpp 直接 include
			// Dialogue/PP_DialogueRequester.h；删掉会编译/链接失败（QuestData、QuestQuery 实际都在 Core）
			"PersonalProjectQuest",
			// 不能删：Public 头 PP_UICombatGateway.h / PP_UIMessagingGateway.h / PP_UILobbyGateway.h 定义在本模块；PP_HUD.cpp 需要 APP_BossCharacter / APP_PlayerCharacter 完整类型，
			// PP_WidgetComponent.cpp 需要 APP_BaseCharacter，PP_LobbyHUD.cpp 需要 APP_CharacterPreviewActor
			"PersonalProject"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			// PP_MenuData.cpp 的 Lua 绑定胶水（TConstructor/FExportedClass）需要 UnLua 运行时符号
			"UnLua",
			"Lua"
		});
	}
}
