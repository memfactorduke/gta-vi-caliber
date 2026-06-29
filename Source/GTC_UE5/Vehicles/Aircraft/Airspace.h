// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Airspace limits for the kinematic flight pawns — the soft ceiling that bleeds away
 * your climb as you near the top of the world, and the restricted floor a no-fly zone
 * imposes. FHelicopterFlight / FFixedWingFlight happily climb forever; this is the rule
 * that keeps an aircraft inside the streamed/rendered volume without a hard wall the
 * player slams into. Instead of clamping position, it scales DOWN the climb the closer
 * you get: full authority below the soft ceiling, fading linearly to zero at the hard
 * ceiling, so the craft mushes to a hover-height near the top and can always dive back.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers (the FVehicleChaseCamera shape). The pawn applies the scaled
 * climb to its integration; reading the actual world ceiling / zone bounds is the adapter.
 *
 * Frame / units: Z is up, heights cm, speeds cm/s. Unit-tested headless
 * (Vehicles/Aircraft/Tests/AirspaceTest.cpp, prefix GTC.Vehicles.Aircraft.Airspace).
 */
struct GTC_UE5_API FAircraftAirspace
{
    /** Per-world tuning. Defaults: soft 2 km, hard 2.5 km above the origin. */
    struct FParams
    {
        /** Hard ceiling Z (cm): climb authority is fully gone at/above this height. */
        double CeilingZCm = 250000.0;
        /** Soft ceiling Z (cm): climb authority is full at/below this, then fades to the
         *  hard ceiling. Clamped to <= CeilingZCm. */
        double SoftCeilingZCm = 200000.0;
    };

    /**
     * Climb authority 0..1 at altitude `CraftZCm`: 1 at/below the soft ceiling, lerping
     * down to 0 at/above the hard ceiling. A degenerate band (soft >= hard) is a hard
     * step: 1 below the ceiling, 0 at/above it.
     */
    static double CeilingAuthority(double CraftZCm, const FParams& Params);

    /**
     * Scale a climb rate by the ceiling authority so ascent fades near the top. Only a
     * POSITIVE (upward) climb is scaled; descent (<= 0) is returned untouched so you can
     * always drop back down out of the thin air.
     */
    static double ClampClimb(double CraftZCm, double ClimbRateCmS, const FParams& Params);

    /** The lowest legal Z (cm) over a no-fly zone: the ground there plus the minimum
     *  altitude. A non-positive minimum just returns the ground height. */
    static double RestrictedFloorZ(double GroundZCm, double MinAltAboveGroundCm);
};
