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
			"PersonalProjectInventory",
			"PersonalProjectQuest",
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
