// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * A campaign: an ordered list of mission definitions the player works through one
 * at a time, the layer above a single MissionController. Tracks which mission is
 * active and advances to the next when one completes.
 *
 * Godot parity: game/scripts/missions/mission_chain.gd (RefCounted). Pure and
 * node-free (unit-tested headless). A coordinator node feeds Current() into a
 * MissionController on start, calls CompleteCurrent() when that controller emits
 * mission_completed, then re-arms with the next Current(); when
 * IsCampaignComplete() it stops re-arming.
 *
 * Typed-port gap: Godot stores each mission as an untyped Dictionary
 * ({id, title, objective_defs, waypoints}). Here that is modelled as FMissionDef,
 * a value struct with a bValid presence flag standing in for Godot's empty-{}
 * "no current mission" sentinel. objective_defs/waypoints are carried verbatim as
 * opaque per-mission data; their interpretation (objectives -> StateTree wiring)
 * is deferred to W2/W3 and not part of this parity port.
 */
class GTC_UE5_API MissionChain
{
public:
    /**
     * One mission definition. bValid distinguishes a real entry from the
     * empty-{} sentinel Current() returns once the campaign is finished.
     */
    struct FMissionDef
    {
        FString Id;
        FString Title;
        /** Opaque carried data (object ids / waypoint keys); not interpreted here. */
        TArray<FString> ObjectiveDefs;
        TMap<FString, FVector> Waypoints;
        bool bValid = false;

        FMissionDef() = default;
        FMissionDef(const FString& InId, const FString& InTitle)
            : Id(InId)
            , Title(InTitle)
            , bValid(true)
        {
        }

        /** Matches Godot's mission.is_empty() on the returned Dictionary. */
        bool IsEmpty() const { return !bValid; }
    };

    /** Copies the input array (Godot duplicate()), so later edits don't alias. */
    explicit MissionChain(const TArray<FMissionDef>& Missions = TArray<FMissionDef>());

    /** How many missions the campaign holds. */
    int32 Count() const;

    /** Index of the active mission (== Count() once every mission is done). */
    int32 ActiveIndex() const;

    /** The active mission definition, or an empty (invalid) def when finished/empty. */
    FMissionDef Current() const;

    /** Id of the active mission, or "" when none remains. */
    FString CurrentId() const;

    /** Close the active mission and arm the next. No-op once the campaign is done. */
    void CompleteCurrent();

    /** Every mission finished (or an empty campaign — nothing to do). */
    bool IsCampaignComplete() const;

    /** Missions still to play, including the active one. */
    int32 Remaining() const;

    /** Missions completed so far. */
    int32 Completed() const;

    /** Campaign progress 0.0 .. 1.0 (1.0 for an empty campaign — already done). */
    double Progress() const;

    /** Restart the campaign from the first mission. */
    void Reset();

private:
    TArray<FMissionDef> Missions;
    int32 Index = 0;
};
