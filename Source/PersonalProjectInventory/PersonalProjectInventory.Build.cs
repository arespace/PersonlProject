// PersonalProjectInventory 模块 — 物品经济域（库存、商店、采集）
// 依赖：PersonalProjectCore（DataManager/EventBus/数据头）

using UnrealBuildTool;

public class PersonalProjectInventory : ModuleRules
{
	public PersonalProjectInventory(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayAbilities",
			"PersonalProjectCore"
		});
	}
}
