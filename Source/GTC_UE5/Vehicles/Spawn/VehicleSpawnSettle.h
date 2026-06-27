// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FVehicleSpawnSettle — the scene-free decision behind "car flies on spawn in a
 * World-Partition map". A physics car spawned at a PlayerStart whose ground cell has
 * not streamed in yet drops into empty space; a frame or few later the road streams
 * in UNDER it and Chaos depenetration launches it skyward. The cure is timing, not
 * physics tuning: each PRE-physics frame the owning component traces straight down
 * and asks this core what to do, so the car is parked a fixed clearance ABOVE its
 * spawn until real ground exists, then dropped to rest just above it — it is never
 * overlapping the road on the frame the road appears, so the launch can't happen.
 *
 * Pure + deterministic: doubles only (cm in this project), no UWorld / no trace / no
 * actor, so it is headless-unit-tested (Tests/VehicleSpawnSettleTest.cpp) exactly
 * like FVehicleEntry. The engine-only half (downward trace, teleport, velocity reset)
 * lives in UVehicleSpawnSettleComponent.
 */
struct FVehicleSpawnSettle
{
    /** Designer tuning, mirrored from the component's editable properties. */
    struct FParams
    {
        /** How far above the spawn Z to park the car while ground has not streamed in. */
        double HoldClearanceCm = 300.0;
        /** Gap left above the hit surface so the wheels drop a touch and rest — never penetrate. */
        double SettleAboveGroundCm = 25.0;
        /** Fail-safe: stop holding after this long if ground never appears (e.g. spawned over a hole). */
        double MaxWaitSeconds = 5.0;
    };

    enum class EAction : uint8
    {
        Hold,   //< Ground not found yet — keep the car parked above its spawn.
        Settle, //< Ground found — snap to just above it and hand control back to physics.
        GiveUp  //< Waited past MaxWaitSeconds — release the car where it is (fail-safe).
    };

    struct FResult
    {
        EAction Action = EAction::Hold;
        /** World Z to move the car's origin to this frame (unused for GiveUp). */
        double TargetZ = 0.0;
        /** True once the component should stop ticking and leave the car to physics. */
        bool bDone = false;
    };

    /**
     * @param bGroundHit     did this frame's downward trace hit blocking ground?
     * @param GroundZ        world Z of that hit (ignored when !bGroundHit)
     * @param SpawnZ         world Z the car spawned at (recorded on the first frame)
     * @param ElapsedSeconds seconds spent waiting for ground so far
     */
    static FResult Evaluate(bool bGroundHit, double GroundZ, double SpawnZ,
        double ElapsedSeconds, const FParams& P)
    {
        // A real hit always wins — settle even if we're already past the timeout.
        if (bGroundHit)
        {
            return FResult{ EAction::Settle, GroundZ + P.SettleAboveGroundCm, /*bDone*/ true };
        }
        if (ElapsedSeconds >= P.MaxWaitSeconds)
        {
            return FResult{ EAction::GiveUp, SpawnZ + P.HoldClearanceCm, /*bDone*/ true };
        }
        return FResult{ EAction::Hold, SpawnZ + P.HoldClearanceCm, /*bDone*/ false };
    }
};
