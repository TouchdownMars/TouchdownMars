// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

using UnrealBuildTool;

public class TouchdownMarsTarget : TargetRules
{
	public TouchdownMarsTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TouchdownMars");
	}
}
