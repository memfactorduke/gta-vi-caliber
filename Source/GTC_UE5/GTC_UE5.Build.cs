// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class GTC_UE5 : ModuleRules
{
	public GTC_UE5(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Unity (jumbo) builds are disabled for this module. With many contributors
		// adding files that each define copy-pasted file-scope test/helper symbols
		// (Eps/Fwd/PosMod/Wrap/Hash01/Banks/...), unity concatenation kept colliding
		// them as blob composition shifted — an unbounded source of redefinition /
		// -Wshadow build breaks. A full non-unity build was verified clean (the module
		// is include-clean, no IWYU regressions), so this is a safe, durable fix:
		// every .cpp compiles as its own TU, immune to cross-file file-scope clashes.
		bUseUnity = false;

		// "UMG": the HUD C++ base (UI/Hud/GTCHudWidget) derives from UUserWidget and subscribes to the
		// W2 player-component change delegates. Public because GTCHudWidget.h includes the UMG UserWidget
		// header in its public API.
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "UMG" });

		// "Slate"/"SlateCore": the ported in-world + front-end UI are pure C++/Slate widgets
		// (UI/Phone/SGTCPhone, UI/Menus/SGTCPauseMenu, FrontEnd/SGTCFrontEnd, FrontEnd/SGTCLoadingScreen).
		// Public because those S-widget headers derive from SCompoundWidget and are included by the
		// player controller / front-end controller inside this module.
		PublicDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

		// Front-end boot flow (intro videos + looping menu + loading cover): the videos play through the
		// Media framework with runtime-created MediaPlayer/MediaTexture (no editor-authored assets), and
		// the loading cover rides the MoviePlayer across the blocking world travel. Private — no
		// Media/MoviePlayer type crosses this module's public boundary.
		PrivateDependencyModuleNames.AddRange(new string[] { "Media", "MediaAssets", "MediaUtils", "MoviePlayer" });

		// "DeveloperSettings": UGTCRadialMenuSettings (UI/Radial) derives from
		// UDeveloperSettings so the radial-menu look/size is tunable live from the Unreal
		// editor's Project Settings (no rebuild, no in-game console). Private — the
		// settings type is read only inside this module (the radial Slate widget).
		PrivateDependencyModuleNames.AddRange(new string[] { "DeveloperSettings" });

		// "Json": engine JSON model used only inside Systems/Save/SaveJson.cpp (FJsonObject /
		// FJsonSerializer / TJsonReader / TJsonWriter). Private — no engine Json type crosses the
		// Save headers' module boundary; the public Save API is the in-module ordered wrapper.
		PrivateDependencyModuleNames.AddRange(new string[] { "Json" });

		// "AIModule" + "NavigationSystem": the living-NPC layer (NPC/Agent) spawns AGTCCitizen
		// pawns that auto-possess a default AAIController so CharacterMovement consumes the
		// steering input (Pawn::AddMovementInput needs a controller to feed the movement
		// component), and so citizens can later path on the world NavMesh. Private — no
		// AIController/Navigation type crosses an NPC public header (the Citizen header only
		// forward-declares engine types and includes the scene-free pure-core NPC structs).
		// Both are standard runtime modules compiled for every target, so the editor-closed
		// headless build stays clean.
		PrivateDependencyModuleNames.AddRange(new string[] { "AIModule", "NavigationSystem" });

		// "PhysicsCore": GTCCitizen loads a flesh UPhysicalMaterial (surface-type lookup for
		// weapon-impact FX). UE 5.8 no longer pulls PhysicsCore in transitively via Engine, so
		// the UPhysicalMaterial UClass symbol (Z_Construct_UClass_UPhysicalMaterial) must be
		// linked explicitly. Private — UPhysicalMaterial is used only inside this module's .cpp.
		PrivateDependencyModuleNames.AddRange(new string[] { "PhysicsCore" });

		// Uncomment if you are using Slate UI
		// PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });
		
		// Uncomment if you are using online features
		// PrivateDependencyModuleNames.Add("OnlineSubsystem");

		// To include OnlineSubsystemSteam, add it to the plugins section in your uproject file with the Enabled attribute set to true
	}
}
