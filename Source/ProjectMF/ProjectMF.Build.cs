// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class ProjectMF : ModuleRules
{
	public ProjectMF(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "Paper2D", "PaperZD",
			// AI 基础导航 (AIController, UPathFollowingComponent)
			"AIModule", "NavigationSystem",
			// TODO(Mass Phase 2): 添加 Mass 模块时取消注释:
			// "MassEntity", "MassCommon", "MassActors", "MassRepresentation", "MassSignals"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {  });

		// Character
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Character", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Character", "Private"));

		// AI
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "AI", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "AI", "Private"));
	}
}
