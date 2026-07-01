// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RivalPacer.h" // FRivalPacer

/**
 * FRaceProgressResolver — the pure glue between UE world state and the race cores
 * (StreetRace / FRaceSession / FRivalPacer). The cores' own headers defer exactly this
 * ("the RaceController adapter builds the FRaceSession from the placed checkpoint ring,
 * drives rival progress from their AI pawns, calls Reached(...) ... — none of that adapter
 * is covered here"). This is that adapter's pure, testable half.
 *
 * Two jobs:
 *   - COORDINATE BRIDGE. StreetRace/FRaceSession reason on the Godot XZ ground plane (Y up)
 *     with NO Z-up remap (their headers say so, and StreetRace::Ground drops Y). UE cars drive
 *     on the XY plane (Z is height). ToRaceSpace routes UE.X->X and UE.Y->Z so the cores measure
 *     checkpoint distance on UE's ground plane; apply it to BOTH the checkpoint ring and the
 *     player position so the two are compared in one space. (Getting this wrong silently makes
 *     gate hits depend on height and ignore one horizontal axis.)
 *   - RIVAL PACING. Turn a rival's pace clock into how many gates it should have cleared by now,
 *     so the session adapter can advance that rival's StreetRace without a real pawn.
 *
 * Pure, deterministic, no UObject. The UObject shell UGTCStreetRaceComponent owns the
 * FRaceSession and only feeds it these results.
 */
struct GTC_UE5_API FRaceProgressResolver
{
    /** Map a UE (Z-up) world position into the race cores' XZ (Y-up) ground-plane convention. */
    static FVector ToRaceSpace(const FVector& UeWorld) { return FVector(UeWorld.X, 0.0, UeWorld.Y); }

    /** A rival's paced state this tick. */
    struct FRivalTick
    {
        double Progress = 0.0;    // monotonic 0..1 race progress
        int32 TargetCleared = 0;  // gates it should have cleared on a TotalCheckpoints course
    };

    /**
     * A rival's progress after Elapsed race seconds (pace clock + bounded rubber-band toward the
     * player, floored at PrevProgress so it never goes backwards), and the gate count that implies.
     */
    static FRivalTick RivalTick(double PrevProgress, double Elapsed, double PaceSeconds,
        double PlayerProgress, double RubberBandStrength, double RubberBandMax, int32 TotalCheckpoints);
};
