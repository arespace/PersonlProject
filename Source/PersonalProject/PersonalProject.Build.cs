// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class PersonalProject : ModuleRules
{
	public PersonalProject(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"GameplayAbilities",
			"GameplayTasks",
			"GameplayTags",
			"UMG",
			"Slate",
			"SlateCore",
			"OnlineSubsystem",
			"OnlineSubsystemUtils",
			"UnLua",
			"Lua",
			"PersonalProjectCore",
			"PersonalProjectInventory",
			"PersonalProjectQuest"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AIModule" });

	}
}
