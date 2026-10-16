// PersonalProjectCore 模块 — 基础设施层（依赖最底层，不依赖任何项目模块）
// 内容：EventBus、EventTypes、DataManager、InteractableInterface、GameplayTags、
//       LocalSaveGame、纯数据头（CharacterData/ItemData/ItemTypes/QuestData/ShopData/DialogueData/ScoreboardTypes）

using UnrealBuildTool;

public class PersonalProjectCore : ModuleRules
{
	public PersonalProjectCore(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"GameplayTags",
			// ItemData 数据资产引用 GameplayEffect 类型（药水回复等），Core 允许依赖 GAS
			"GameplayAbilities"
		});
	}
}
