// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FRailPathResolver — the pure "map a distance-along-the-line to a world transform on the spline"
 * adapter half for the train. FTrainMover is 1-DOF (a distance + speed along a looping line) and
 * defers "placing the track spline, mapping Position to a world transform" to its adapter. This is
 * that map: given the rail as a LOOPING polyline of world points and a distance in cm, walk the
 * polyline (wrapping past the end) to the world position there and the forward tangent (the
 * direction of travel, for facing the carriage).
 *
 * The line is a loop: the polyline's last point connects back to its first. The shell sets the
 * train's FParams.TrackLength to LoopLength(Points) so the 1-DOF distance maps 1:1 onto the geometry.
 * Pure, deterministic, no UObject. Units: points/distances in cm.
 */
struct GTC_UE5_API FRailPathResolver
{
    struct FSample
    {
        FVector Position = FVector::ZeroVector; // world position at the distance
        FVector Forward = FVector(1.0, 0.0, 0.0); // unit forward tangent (direction of travel)
        bool bValid = false;                     // false if the path is degenerate (<2 points / zero length)
    };

    /** Total loop length of the polyline: every segment plus the closing segment back to the start.
     *  0 if fewer than 2 points. */
    static double LoopLength(const TArray<FVector>& Points);

    /** World transform at `Distance` cm along the looping polyline (Distance wraps modulo the loop
     *  length; negative distances wrap too). bValid is false for a degenerate path. */
    static FSample Sample(const TArray<FVector>& Points, double Distance);
};
