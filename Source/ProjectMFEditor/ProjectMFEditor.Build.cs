// Copyright ProjectMF. All Rights Reserved.

using UnrealBuildTool;

public class ProjectMFEditor : ModuleRules
{
	public ProjectMFEditor(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] {
			"Core", "CoreUObject", "Engine", "ProjectMF"
		});

		PrivateDependencyModuleNames.AddRange(new string[] {
			"UnrealEd", "PropertyEditor", "Slate", "SlateCore", "InputCore"
		});
	}
}
