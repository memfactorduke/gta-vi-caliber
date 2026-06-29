// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/** Which medium the player is currently fleeing through — drives air/sea response. */
enum class EPlayerMedium : uint8
{
    Land,
    Air,
    Sea,
};

/**
 * Air + sea police escalation — WHEN the pursuit helicopter scrambles and WHEN the new
 * Coast Guard interceptors launch, keyed on both the wanted stars AND what the player is
 * fleeing in. A chopper joins a road chase at the usual heat, but it scrambles SOONER for
 * an airborne player (you stole a helicopter — they answer in kind), and the Coast Guard
 * only matters when you're out on the water. It sits beside FPoliceEscalation /
 * FPoliceResponse and feeds the streaming directors' deploy gates.
 *
 * Reconciles an existing discrepancy: FPoliceResponse::HelicopterStars == 3 but
 * FPoliceEscalation::HelicopterStars == 4. The player-facing rule (and the actual
 * AGTCPoliceHelicopter deploy) is 3, so PoliceAirStars == 3 is treated as canonical here;
 * the escalation table's 4 is its SWAT-by-air-insertion tier, a separate thing.
 *
 * GTC-original pure-core: scene-free, deterministic, no UObject — stateless static helpers,
 * clamped and monotonic like the sibling escalation cores. Units: stars are integers
 * (clamped 0..MaxStars). Unit-tested headless
 * (AI/PoliceDispatch/Tests/AirSeaEscalationTest.cpp, prefix GTC.AI.PoliceDispatch.AirSeaEscalation).
 */
struct GTC_UE5_API FAirSeaEscalation
{
    /** Highest star level the curves consider. */
    static constexpr int32 MaxStars = 6;

    /** Canonical star at which a pursuit helicopter joins (matches FPoliceResponse). */
    static constexpr int32 PoliceAirStars = 3;

    /** Star at which the Coast Guard launches against a waterborne player. */
    static constexpr int32 CoastGuardStars = 4;

    /** Clamp a raw star count to [0, MaxStars]. */
    static int32 ClampStars(int32 Stars);

    /**
     * Whether a police helicopter should be airborne now: at/above PoliceAirStars on land
     * or sea, and one star EARLIER when the player is airborne (scramble to meet a flyer).
     * Never below 1 star.
     */
    static bool ShouldDeployPoliceAir(int32 Stars, EPlayerMedium Medium);

    /** Whether the Coast Guard should launch: only when the player is at Sea and at/above
     *  CoastGuardStars. */
    static bool ShouldDeployCoastGuard(int32 Stars, EPlayerMedium Medium);

    /** How many police helicopters at this heat (0 when not deployed; 1, then 2 at 5+). */
    static int32 PoliceAirCount(int32 Stars, EPlayerMedium Medium);

    /** How many Coast Guard boats at this heat (0 when not deployed; rises with stars, capped). */
    static int32 CoastGuardBoatCount(int32 Stars, EPlayerMedium Medium);
};
