// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GTC_UE5 : ModuleRules
{
	public GTC_UE5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput" });

		// "Json": engine JSON model used only inside Systems/Save/SaveJson.cpp (FJsonObject /
		// FJsonSerializer / TJsonReader / TJsonWriter). Private — no engine Json type crosses the
		// Save headers' module boundary; the public Save API is the in-module ordered wrapper.
		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
