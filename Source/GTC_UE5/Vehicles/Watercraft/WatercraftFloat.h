// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Kinematic wave-riding pose for a boat — the vertical + tilt half of watercraft, the
 * sibling to FBoatHandling's planar half. FBoatHandling drives the boat across the water
 * (surge/sway/heading, Z = 0); this sits the hull ON the water: given the ocean surface
 * height under a handful of hull sample points, it fits a plane to them and reads off the
 * resting height (floating at its draft), the pitch + roll that match the swell, how rough
 * the water under the hull is, and whether a wave face is steep enough to capsize.
 *
 * It's the arcade alternative to a full buoyancy force-integrator (FOceanBuoyancy +
 * rigid body): no Chaos, no mass/inertia tuning — a boat planing across chop just tracks
 * the surface and tilts with it, which is the feel a GTA-style speedboat wants. The
 * adapter samples FOceanSurface::HeightAt at each hull point (converting m->cm), calls
 * ResolvePose, and writes the Z + rotation onto the kinematic pawn.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject — a
 * least-squares plane fit (Cramer's rule, with a level fallback for degenerate samples).
 * The hull sample LAYOUT is boat-local (X forward, Y right); the caller transforms them to
 * world to read the surface, so no heading is needed in here. Z is up; positions/heights
 * cm, angles radians. Unit-tested headless (Vehicles/Watercraft/Tests/WatercraftFloatTest.cpp,
 * prefix GTC.Vehicles.Watercraft.Float).
 */
struct GTC_UE5_API FWatercraftFloat
{
    /** A hull contact sample in the boat's local frame (X forward, Y right), cm. */
    struct FHullSample
    {
        double Xcm = 0.0;
        double Ycm = 0.0;
    };

    /** Per-hull tuning. */
    struct FParams
    {
        /** How far (cm) the hull floats below the surface — the waterline draft. */
        double DraftCm = 40.0;
        /** |pitch| or |roll| (radians) beyond which the hull is considered capsized. */
        double CapsizeAngleRad = 1.2;
        /** Surface height spread (cm, max-min across samples) that reads as fully rough
         *  water (Roughness01 == 1). */
        double RoughnessRefCm = 200.0;
    };

    /** The resting pose to drive the kinematic boat with. */
    struct FPose
    {
        /** World Z (cm) for the boat origin: the fitted surface under the centre, minus draft. */
        double Zcm = 0.0;
        /** Pitch (radians): nose up when the swell rises toward the bow. */
        double PitchRad = 0.0;
        /** Roll (radians): starboard up when the swell rises to the right. */
        double RollRad = 0.0;
        /** 0..1 how uneven the surface under the hull is (spray / instability feedback). */
        double Roughness01 = 0.0;
        /** True when a wave face tilts the hull past CapsizeAngleRad. */
        bool bCapsized = false;
    };

    /**
     * Fit the resting pose to the ocean surface under the hull. `SurfaceHeightsCm[i]` is
     * the surface height (cm, Z-up, sea level 0) at hull sample `Samples[i]`. `Count` must
     * match both arrays. With no samples, or samples that don't span a plane (a single
     * point / a line), it returns a level pose at the mean height minus draft.
     */
    static FPose ResolvePose(const double* SurfaceHeightsCm, const FHullSample* Samples,
        int32 Count, const FParams& Params);
};
