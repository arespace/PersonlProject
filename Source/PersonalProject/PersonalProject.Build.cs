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
			// Public 头 PP_PlayerController.h 持有 TSubclassOf<UUserWidget>，UMG 必须公开（Slate/SlateCore 由 UMG 传递）
			"UMG",
			// Public 头 PP_SessionManager.h 持有 IOnlineSessionPtr，OnlineSubsystem 必须公开
			"OnlineSubsystem",
			"PersonalProjectCore",
			"PersonalProjectInventory",
			"PersonalProjectQuest",
			// ALS 集成 POC：APP_ALSPlayerCharacter 继承 AAlsCharacterExample
			"ALS",
			"ALSExtras",
			"ALSCamera"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "AIModule" });

		// 编辑器专用工具（PP_AlsRetargetTool.cpp）：复制动画蓝图、切换骨架、重映射动画引用
		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"UnrealEd",
				"AssetTools",
				"AssetRegistry"
			});
		}

	}
}
