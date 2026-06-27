// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class GTC_UE5EditorTarget : TargetRules
{
	public GTC_UE5EditorTarget( TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		DefaultBuildSettings = BuildSettingsVersion.V7;
		// Stay on the 5.7 include-order compatibility shim under the 5.8 engine
		// (Epic-supported): keeps deprecated transitive includes alive so the
		// existing source compiles without a full include-order migration.
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_7;
		// The installed UE_5.8 UnrealEditor was built with these warning levels
		// Off; sharing its build products requires us to either match them or
		// force the override. We force the override so we can keep our own
		// settings while still linking the precompiled engine (no ABI impact).
		bOverrideBuildEnvironment = true;
		// ...but DO NOT promote these to errors: the V7 defaults turn them into
		// hard errors, which makes a full build fail inside vendored third-party
		// plugins (e.g. VibeUE -Wunreachable-code-loop-increment). A failed full
		// build never regenerates the .target receipt, so the editor prompts
		// "rebuild required" on every launch. Keep them as warnings so the full
		// build completes and the receipt stays in sync with this source.
		CppCompileWarningSettings.UnreachableCodeWarningLevel = WarningLevel.Warning;
		CppCompileWarningSettings.ReturnTypeWarningLevel = WarningLevel.Warning;
		CppCompileWarningSettings.DanglingWarningLevel = WarningLevel.Warning;
		ExtraModuleNames.Add("GTC_UE5");
	}
}
