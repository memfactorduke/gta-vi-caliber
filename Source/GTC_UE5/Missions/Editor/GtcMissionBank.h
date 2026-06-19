// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FGtcMissionDefinition;
struct FGtcObjectiveDefinition;
enum class EGtcMissionContentType : uint8;
enum class EGtcObjectiveKind : uint8;

/**
 * The Mission Editor's authoring LOGIC — pure C++ free functions over a bank (TArray) of
 * FGtcMissionDefinition, plus a validator. No UObject, no world: exactly the scene-free
 * pure-core style used by Missions/Core (Mission.h) and the save system (GtcSaveData), so
 * every authoring operation and every validation rule is unit-tested headlessly.
 *
 * The UObject layer (UGtcMissionEditorSubsystem) owns the reflected `TArray` the god-mode
 * UI binds to and simply forwards into these functions — it adds reflection and input
 * surface, never logic.
 */

/** Result of validating one mission definition. Errors block playback; warnings don't. */
struct GTC_UE5_API FGtcMissionValidation
{
    TArray<FString> Errors;
    TArray<FString> Warnings;

    bool IsValid() const { return Errors.Num() == 0; }
};

namespace GtcMissionBank
{
    /**
     * Append a fresh, sensibly-defaulted mission of the given type and return its new
     * stable MissionId. Activities default to a Side Job payload.
     */
    GTC_UE5_API FGuid CreateMission(TArray<FGtcMissionDefinition>& Bank, EGtcMissionContentType Type);

    /** Find a mission by stable id (nullptr if absent). */
    GTC_UE5_API FGtcMissionDefinition* Find(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId);
    GTC_UE5_API const FGtcMissionDefinition* Find(const TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId);

    /** Remove a mission by id. True if one was removed. */
    GTC_UE5_API bool RemoveMission(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId);

    /** Append an objective to a mission. False if the mission is absent. */
    GTC_UE5_API bool AddObjective(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, const FGtcObjectiveDefinition& Objective);

    /** Remove every objective with the given id from a mission. True if any were removed. */
    GTC_UE5_API bool RemoveObjective(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, const FString& ObjectiveId);

    /** Move the objective at FromIndex to ToIndex within a mission. False on bad indices. */
    GTC_UE5_API bool ReorderObjective(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, int32 FromIndex, int32 ToIndex);

    /**
     * Set an objective's waypoint to a world location and mark it as having one — the data
     * write behind god-mode "place this objective here". False if the mission or objective
     * id is absent.
     */
    GTC_UE5_API bool SetObjectiveWaypoint(TArray<FGtcMissionDefinition>& Bank, const FGuid& MissionId, const FString& ObjectiveId, const FVector& WorldLocation);

    /**
     * Whether a mission should currently offer a world start-marker: it must opt in
     * (bAvailableInWorld), all its prerequisite missions must be in Completed, and it must not
     * already be completed (unless bRepeatable). Pure so the gating is unit-tested.
     */
    GTC_UE5_API bool IsMissionAvailable(const FGtcMissionDefinition& Def, const TSet<FGuid>& Completed);

    /** Count-based objective kinds (advanced by NotifyObjectiveProgress): Collect and Eliminate. */
    GTC_UE5_API bool IsCountObjective(EGtcObjectiveKind Kind);

    /**
     * Validate one definition. Consolidates the runtime gotchas the codebase maps flagged:
     * empty title, zero objectives (a narrative mission with none never completes),
     * duplicate/empty objective ids, bad DriverKind, Collect/Eliminate count < 1, negative
     * time, activity-specific checks (empty race, same pickup/dropoff), and advisory notes
     * (negative reward no-ops, unlocks beyond the hardcoded gate table).
     */
    GTC_UE5_API FGtcMissionValidation Validate(const FGtcMissionDefinition& Def);
}
