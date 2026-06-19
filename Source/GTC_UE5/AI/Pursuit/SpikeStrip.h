// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The spike strip (stinger) — police fling a barbed strip across the road and a fleeing car
 * that drives over it shreds its tyres, killing its grip and top speed. Where FRoadblock is a
 * wall to thread or ram, the stinger is a hazard to swerve around: cross it and the chase
 * tilts hard in the cops' favour. It's the geometric trigger (did the car's path this frame
 * cross the strip line) plus the deflation effect (multipliers that feed
 * FVehicleHandling / FVehicleCondition grip + top-speed).
 *
 * PURE-CORE: scene-free, deterministic, double precision, FVector-in, no engine coupling, no
 * RNG. The pursuit adapter lays a strip (two endpoints, or a centre + road-right + width),
 * each frame tests Crosses(prevPos, currPos, A, B) for the active vehicle, and on a hit
 * applies DeflatedGripMult / DeflatedTopSpeedMult to that vehicle's handling/condition (and
 * spawns the tyre-burst FX). Ground-plane math uses X/Y (Z ignored), matching the project's
 * no-Z-up-remap convention. Distances are the adapter's own units (UE cm). Unit-tested
 * headless (AI/Pursuit/Tests/SpikeStripTest.cpp, prefix GTC.AI.Pursuit.SpikeStrip).
 */
struct GTC_UE5_API FSpikeStrip
{
    /** Lateral grip multiplier once the tyres are shredded (a fraction of normal grip). */
    static constexpr double DeflatedGripMult = 0.4;
    /** Top-speed multiplier on shredded tyres. */
    static constexpr double DeflatedTopSpeedMult = 0.55;

    /**
     * Did the vehicle's travel this frame (the segment PrevPos->CurrPos) cross the strip
     * segment StripA->StripB, on the ground plane (X/Y)? Proper segment intersection;
     * a merely-touching/collinear pass is treated as no crossing (the adapter retests next
     * frame). Zero-length inputs return false.
     */
    static bool Crosses(const FVector& PrevPos, const FVector& CurrPos,
        const FVector& StripA, const FVector& StripB);

    /**
     * Strip endpoints from a centre, the road's right vector (need not be normalised), and the
     * road width: A = Centre - Right*Width/2, B = Centre + Right*Width/2. A degenerate Right
     * collapses both to Centre.
     */
    static void Endpoints(const FVector& Center, const FVector& RoadRight, double Width,
        FVector& OutA, FVector& OutB);
};
