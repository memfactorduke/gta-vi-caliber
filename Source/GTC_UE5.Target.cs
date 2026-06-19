// Copyright (c) 2026 GTC contributors

using UnrealBuildTool;
using System.Collections.Generic;

public class GTC_UE5Target : TargetRules
{
	public GTC_UE5Target(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		ExtraModuleNames.Add("GTC_UE5");
	}
}
