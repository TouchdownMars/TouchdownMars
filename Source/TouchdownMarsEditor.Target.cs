// Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.

using UnrealBuildTool;

public class TouchdownMarsEditorTarget : TargetRules
{
	public TouchdownMarsEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.Latest;
		IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
		ExtraModuleNames.Add("TouchdownMars");
	}
}
