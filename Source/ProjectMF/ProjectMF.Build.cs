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
			// GAS (Gameplay Ability System)
			"GameplayAbilities", "GameplayTags", "GameplayTasks",
			// StateTree (UStateTreeComponent, UStateTreeAIComponent)
			"StateTreeModule", "GameplayStateTreeModule",
			// TODO(Mass Phase 2): 添加 Mass 模块时取消注释:
			// "MassEntity", "MassCommon", "MassActors", "MassRepresentation", "MassSignals"
		});

		PrivateDependencyModuleNames.AddRange(new string[] { "Voronoi" });

		// Character
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Character", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Character", "Private"));

		// AI
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "AI", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "AI", "Private"));

		// GAS
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "GAS", "Public"));
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "GAS", "Public", "AbilityTasks"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "GAS", "Private"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "GAS", "Private", "AbilityTasks"));

		// Core (MFLog, 全局工具)
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Core", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Core", "Private"));

		// Catching (抓宠系统)
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Catching", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Catching", "Private"));

		// Inventory (背包系统)
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Inventory", "Public"));
		PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Inventory", "Private"));

        //
        PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "NMap", "Public"));
        PrivateIncludePaths.Add(Path.Combine(ModuleDirectory, "Nmap", "Private"));
    }
}
