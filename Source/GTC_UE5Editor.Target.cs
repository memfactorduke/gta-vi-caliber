// Copyright (c) 2026 GTC contributors

using UnrealBuildTool;
using System.Collections.Generic;

public class GTC_UE5EditorTarget : TargetRules
{
	public GTC_UE5EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("GTC_UE5");
	}
}
