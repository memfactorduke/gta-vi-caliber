// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "../Core/MissionChain.h"
#include "../Coordinators/MissionObjectiveDriver.h"

struct FGtcMissionDefinition;

/**
 * Translates the editor's reflected authoring model (FGtcMissionDefinition) into the plain
 * inputs the existing runtime consumes — the bridge that makes an authored mission actually
 * runnable. Pure and scene-free (no UObject), so the translation is unit-tested headlessly
 * and can even be fed straight into the pure MissionChain to prove a campaign progresses.
 *
 * The UObject layer (UGtcMissionEditorSubsystem::PlayMission) takes a compiled mission and
 * copies it into the live UMissionController (Title / ObjectiveDefs / Waypoints / TimeLimit)
 * then calls Reset()+Begin(); none of that logic lives here.
 */

/** One compiled objective row — the {id,text} the controller needs, plus its waypoint. */
struct GTC_UE5_API FGtcCompiledObjective
{
    FString Id;
    FString Text;
    bool bHasWaypoint = false;
    FVector Waypoint = FVector::ZeroVector;
};

/** A narrative mission compiled into controller-ready inputs. */
struct GTC_UE5_API FGtcCompiledMission
{
    FString Id;
    FString Title;
    TArray<FGtcCompiledObjective> Objectives;
    double TimeLimit = 0.0;
    /** False when the source mission can't run (invalid, or no objectives to complete). */
    bool bValid = false;
};

namespace GtcMissionCompiler
{
    /** Translate one authored narrative mission into controller-ready form. */
    GTC_UE5_API FGtcCompiledMission CompileMission(const FGtcMissionDefinition& Def);

    /**
     * Translate a sequence of authored missions into a MissionChain campaign (objective ids
     * carried as the opaque per-mission data, waypoints mapped by objective id). The result
     * is ready to hand to `MissionChain(...)`.
     */
    GTC_UE5_API TArray<MissionChain::FMissionDef> CompileCampaign(const TArray<FGtcMissionDefinition>& Defs);

    /**
     * Build the reach/hold satisfaction defs (keyed by objective id) for a mission's objectives
     * that have a waypoint — what FMissionObjectiveDriver uses to auto-complete an objective from
     * player proximity. Objectives without a waypoint (and count/interact kinds) are excluded, so
     * they aren't auto-satisfied. DriverKind other than "hold" maps to "reach".
     */
    GTC_UE5_API TMap<FString, FMissionObjectiveDriver::FObjectiveDef> CompileObjectiveDrivers(const FGtcMissionDefinition& Def);
}
