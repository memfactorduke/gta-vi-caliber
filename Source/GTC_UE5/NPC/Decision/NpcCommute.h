// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Whether a citizen DRIVES to a far destination instead of walking it — the
 * decision half of "walk to a parked car, get in, and drive home". Some people own
 * a car (a seeded fraction of the population); when one of them has a long trip to
 * a commute anchor (home, the office), they head for a parked car and drive rather
 * than trudge across the whole map on foot. Short hops are still walked.
 *
 * This owns the gameplay DECISION and the stage timings; the actor (AGTCCitizen)
 * owns the FSM that walks to the car, hides into it, hands a directed car to the
 * traffic layer, and steps back out on arrival. Keeping the choice + durations here
 * makes them headless-testable without spawning a car or a pawn.
 *
 * PURE-CORE: scene-free, deterministic from seed + inputs, no engine coupling.
 * Unit-tested headless (Tests/NpcCommuteTest.cpp, prefix GTC.NPC.Decision.NpcCommute).
 */

/** The actor's driving mini-FSM. None = on foot as usual. */
enum class ENpcDriveStage : uint8
{
    None,         // not driving — ordinary pedestrian
    WalkingToCar, // heading to the parked car on foot
    Entering,     // at the car, getting in (a brief dwell)
    Driving,      // the traffic layer is driving the car to the destination
    Exiting,      // arrived, getting out (a brief dwell)
};

struct GTC_UE5_API FNpcCommute
{
    /** Seconds spent getting into the car before the drive begins. */
    static constexpr double EnterSeconds = 1.5;
    /** Seconds spent getting out of the car after arrival. */
    static constexpr double ExitSeconds = 1.5;
    /** A trip must be at least this far (cm) to be worth driving — roughly a city
     *  block and a half; shorter and people just walk. */
    static constexpr double DriveThresholdCm = 9000.0;

    /** True if this citizen owns a car (a deterministic ~1-in-3 of the population). */
    static bool HasCar(int32 Seed);

    /**
     * True if the citizen should drive this trip rather than walk it: they own a car,
     * the destination is a commute anchor (home / office), and it's far enough to be
     * worth it. `DestinationPlace` is the FNpcIntent place token; `TripDistanceCm` is
     * the planar distance from the citizen to that destination.
     */
    static bool ShouldDrive(bool bHasCar, const FString& DestinationPlace, double TripDistanceCm);

    /** Dwell duration for a stage (Entering/Exiting); 0 for stages with no dwell. */
    static double StageDuration(ENpcDriveStage Stage);

    /** True once the time spent in a dwelling stage (Entering/Exiting) is up. */
    static bool StageDwellElapsed(ENpcDriveStage Stage, double ElapsedSeconds);
};
