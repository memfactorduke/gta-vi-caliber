// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FDeliveryRunResolver — the pure "cook world state -> FDeliveryRun inputs" adapter half.
 * FDeliveryRun.h names exactly this the DEFERRED adapter: "measuring route progress, turning
 * crashes into cargo Damage ... is NOT covered by the [core's] tests." This is that
 * measurement, extracted as a deterministic struct so it IS tested (headless + oracle) — the
 * same FVehicleGripResolver split the codebase uses (pure resolver + thin UObject shell).
 *
 * Route progress ramps 0..1 as the drop is approached and reaches 1.0 once inside the drop's
 * ARRIVAL RADIUS — you never coincide EXACTLY with an authored point, so arrival is a radius,
 * the same way the mission system's "reach" objective completes within a radius. Progress is a
 * MONOTONIC high-water mark: only NEW ground counts as ProgressDelta, so driving away from the
 * drop and back does not re-award the same stretch (the delivery analogue of a reversing car
 * spuriously scoring drift). Cargo Damage is this tick's accumulated impact impulse scaled into
 * 0..1 cargo loss.
 *
 * Units: distances in the caller's units (UE cm), impulse in the caller's units. The outputs
 * Progress / ProgressDelta / Damage are 0..1 and feed FDeliveryRun::Update directly.
 */
struct GTC_UE5_API FDeliveryRunResolver
{
    struct FParams
    {
        /** Cargo condition (0..1) lost per unit of impact impulse. */
        double DamagePerImpulse = 0.0002;

        /** Distance (cm) from the drop counted as "arrived" — progress reaches 1.0 here, so a
         *  pawn parked near (not exactly on) the marker still delivers. */
        double ArrivalRadius = 500.0;
    };

    struct FInput
    {
        double DistanceToDropoff = 0.0; // current straight-line distance to the drop
        double RouteLength = 0.0;       // distance the drop was from the start (the full route)
        double PrevProgress = 0.0;      // 0..1 high-water progress from last tick (stored by the shell)
        double ImpactImpulse = 0.0;     // impact magnitude accumulated this tick (>= 0)
    };

    struct FOutput
    {
        double Progress = 0.0;      // 0..1 new high-water progress (store as next PrevProgress)
        double ProgressDelta = 0.0; // >= 0 NEW route fraction gained this tick (feed FDeliveryRun)
        double Damage = 0.0;        // >= 0 cargo condition lost this tick (feed FDeliveryRun)
    };

    /** Cook one frame of world state into the delivery core's inputs. Pure + deterministic. */
    static FOutput Cook(const FInput& In, const FParams& P);
};
