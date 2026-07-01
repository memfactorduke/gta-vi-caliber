// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FTailResolver — the pure "cook world state -> FTailObjective inputs" adapter half. Each tick
 * FTailObjective wants the distance to the mark and whether the follower is in the mark's VIEW;
 * its header defers exactly this ("measuring the distance, computing the target's view cone ... is
 * the DEFERRED adapter"). This is that measurement, as a deterministic struct so it IS tested
 * (headless + oracle) — the project's pure-core + thin-shell split.
 *
 * The view test is a forward cone on the GROUND PLANE (yaw only — the mark's pitch on a ramp
 * doesn't tilt the cone, and height on a multi-level map doesn't fold into the tail distance): the
 * follower is "in view" when it is within ViewRange of the mark AND inside the mark's forward
 * half-angle. It is geometry only — true line-of-sight (walls) is an optional engine-side
 * refinement the shell/BP can AND in; the cone is the pure part.
 *
 * Units: positions/distance in cm; ViewHalfAngleRad in radians; ViewRange in cm.
 */
struct GTC_UE5_API FTailResolver
{
    struct FParams
    {
        /** Half-angle of the mark's forward view cone (radians). Default ~45 deg (a 90 deg cone). */
        double ViewHalfAngleRad = 0.7853981634;
        /** Beyond this distance (cm) the mark can't make the follower out, whatever the angle. */
        double ViewRange = 4000.0;
    };

    struct FInput
    {
        FVector FollowerPos = FVector::ZeroVector;   // the tailing player
        FVector TargetPos = FVector::ZeroVector;     // the mark being followed
        FVector TargetForward = FVector::ZeroVector; // the mark's facing (typically ground-plane)
    };

    struct FOutput
    {
        double Distance = 0.0; // follower -> mark, cm
        bool bInView = false;  // inside the mark's forward view cone AND within range
    };

    /** Cook one frame of world state into FTailObjective's (Distance, bInTargetView). Pure. */
    static FOutput Cook(const FInput& In, const FParams& P);
};
