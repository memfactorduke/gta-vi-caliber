// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** The kind of fast-travel hub a marker actor registers as. */
enum class EHubKind : uint8
{
    Helipad,
    Marina,
    Airstrip,
    Generic,
};

/**
 * The fast-travel hub graph — the pure decisions behind "open the map, pick a helipad/
 * marina/airstrip across the bay, smash-cut there". Helipads, marinas and airstrips
 * register as hubs; this answers which hub is nearest (overall or of a kind), what a hop
 * costs, how long it would take, and — importantly — whether you're even allowed to leave
 * (no fast travel while the cops are on you). It is the connective tissue that makes the
 * Wings & Waves hubs more than spawn points.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject —
 * stateless static helpers over a hub list the coordinator owns. The fade-out / relocate /
 * fade-in and the actual UWantedSubsystem read are the deferred coordinator adapter.
 *
 * Frame / units: Z is up, positions cm, fares whole currency units, times seconds, the
 * threat range metres (to match the wanted/perception layer). Unit-tested headless
 * (World/FastTravel/Tests/FastTravelNetworkTest.cpp, prefix GTC.World.FastTravel).
 */
struct GTC_UE5_API FFastTravelNetwork
{
    /** Returned when no hub qualifies. */
    static constexpr int32 None = -1;

    /** One registered hub. */
    struct FHub
    {
        FVector Location = FVector::ZeroVector;
        EHubKind Kind = EHubKind::Generic;
        int32 Id = 0;
        bool bUnlocked = true;
    };

    /**
     * Index of the nearest UNLOCKED hub to `From` within `MaxRangeCm` (a non-positive
     * range means unlimited), or None. Ties resolve to the lower index.
     */
    static int32 NearestHub(const TArray<FHub>& Hubs, const FVector& From, double MaxRangeCm);

    /** As NearestHub, but only considering hubs of `Kind`. */
    static int32 NearestHubOfKind(const TArray<FHub>& Hubs, const FVector& From, EHubKind Kind, double MaxRangeCm);

    /**
     * Fare for a hop From -> To: `BaseFare` plus `PerKm` per kilometre of straight-line
     * distance, rounded to a whole unit. `PerKm`/`BaseFare` clamp to >= 0, so the fare is
     * never below the base.
     */
    static int32 Fare(const FHub& From, const FHub& To, double PerKm, int32 BaseFare);

    /** Travel time (seconds) for a hop at `AssumedSpeedCmS` (clamped to a tiny minimum). */
    static double TravelSeconds(const FHub& From, const FHub& To, double AssumedSpeedCmS);

    /**
     * Whether a fast-travel departure is allowed: never while `bWanted`, and never while a
     * threat is within `SafeRangeM` metres (`NearestThreatM` is the distance to the closest
     * one; pass a large value when clear).
     */
    static bool CanDepart(bool bWanted, double NearestThreatM, double SafeRangeM);
};
