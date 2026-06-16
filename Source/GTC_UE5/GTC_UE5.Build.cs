// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GTC_UE5 : ModuleRules
{
	public GTC_UE5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		// "UMG": the HUD C++ base (UI/Hud/GTCHudWidget) derives from UUserWidget and subscribes to the
		// W2 player-component change delegates. Public because GTCHudWidget.h includes the UMG UserWidget
		// header in its public API. Slate/SlateCore are NOT added: the C++ base touches no Slate types
		// directly (the meta=(BindWidget) ProgressBars/TextBlocks live in the future WBP, not in this base).
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });

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
