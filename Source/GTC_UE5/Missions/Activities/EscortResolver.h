// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FEscortResolver — the pure "cook world state -> FEscortObjective inputs" adapter half. Each tick
 * FEscortObjective wants the escortee's route progress and the THREAT LEVEL on it; its header defers
 * exactly that ("measuring the escortee's route progress, computing the threat level from attackers
 * ... is the DEFERRED adapter"). This is that measurement, as a deterministic struct so it IS tested.
 *
 * Two jobs:
 *   - ROUTE PROGRESS, monotonic high-water toward the destination (reaches 1 within ArrivalRadius),
 *     so a convoy that stalls, weaves, or is pushed back doesn't lose ground it already covered.
 *   - THREAT, cooked from nearby attackers: each hostile within ThreatRadius adds heat that grows as
 *     it closes (1 at point blank, 0 at the edge), and the sum saturates to 1 at ThreatSaturation
 *     point-blank attackers. That 0..1 is what drains (or, when near zero, lets recover) integrity.
 *
 * Pure, deterministic, no UObject. Units: distances/radii in cm; Progress/ProgressDelta/ThreatLevel
 * are 0..1 and feed FEscortObjective::Update directly.
 */
struct GTC_UE5_API FEscortResolver
{
    struct FParams
    {
        /** Distance (cm) from the destination counted as arrived (progress reaches 1 here). */
        double ArrivalRadius = 500.0;
        /** Hostiles beyond this distance (cm) from the escortee pose no threat. */
        double ThreatRadius = 3000.0;
        /** Proximity-weighted point-blank attacker count that saturates threat to 1. */
        double ThreatSaturation = 3.0;
    };

    struct FInput
    {
        FVector EscorteePos = FVector::ZeroVector;   // the convoy being protected
        FVector Destination = FVector::ZeroVector;   // where it's headed
        double RouteLength = 0.0;                    // escortee start -> destination distance
        double PrevProgress = 0.0;                   // 0..1 high-water progress from last tick
        TArray<FVector> HostilePositions;            // nearby attacker positions
    };

    struct FOutput
    {
        double Progress = 0.0;      // 0..1 new high-water progress (store as next PrevProgress)
        double ProgressDelta = 0.0; // >= 0 new route fraction covered this tick
        double ThreatLevel = 0.0;   // 0..1 heat on the escortee right now
    };

    /** Cook one frame of world state into the escort core's inputs. Pure + deterministic. */
    static FOutput Cook(const FInput& In, const FParams& P);
};
