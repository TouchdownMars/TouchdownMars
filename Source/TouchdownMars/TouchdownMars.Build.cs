// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

using UnrealBuildTool;

public class TouchdownMars : ModuleRules
{
	public TouchdownMars(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Sub-folders (Model/, Systems/, Actors/, Tests/) are included as "Model/TDTypes.h".
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"UMG",
			"Slate",
			"SlateCore",
			"HTTP",
			"Json",
			"JsonUtilities",
			"DeveloperSettings",
			"CinematicCamera",
			"NavigationSystem",
			"AIModule",
			"Landscape"
		});
	}
}
