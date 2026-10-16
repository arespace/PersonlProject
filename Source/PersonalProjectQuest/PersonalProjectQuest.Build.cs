// PersonalProjectQuest 模块 — 任务对话域（任务组件、事件桥接、NPC 对话）
// 依赖：PersonalProjectCore（DataManager/EventBus）、PersonalProjectInventory（发奖/商店条件，阶段A豁免保留）

using UnrealBuildTool;

public class PersonalProjectQuest : ModuleRules
{
	public PersonalProjectQuest(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"PersonalProjectCore",
			"PersonalProjectInventory"
		});
	}
}
