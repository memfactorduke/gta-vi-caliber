// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * How a citizen responds to a gun in their face. The plain threat read (FNpcReaction)
 * already empties a street when the player draws a weapon — everyone runs. But a
 * weapon levelled point-blank is different: running gets you shot, so a frightened
 * person freezes, throws their hands up, and complies, while a steadier-nerved one
 * still bolts for cover. This is the pure decision for that split — the detail that
 * separates "armed player => generic flee" from a city that cowers when you stick a
 * gun in someone's ribs and scatters when you wave it down the block.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling. The
 * adapter feeds distance (metres, like FNpcReaction/FNpcMemory), whether the player
 * is brandishing (UGTCCrowdSubsystem::SetPlayerArmed / the weapon layer), and the
 * citizen's bravery. Unit-tested headless (Tests/NpcDuressTest.cpp, prefix
 * GTC.NPC.Decision.NpcDuress).
 */
enum class ENpcDuress : uint8
{
    None,  // not a gun-duress situation (unarmed, or too far to matter yet)
    Flee,  // run from the weapon — the medium-range / brave response
    Cower, // frozen, hands up, complying — too close and too scared to run
};

struct GTC_UE5_API FNpcDuress
{
    /** Beyond this range (m) a brandished weapon isn't yet an immediate duress (the
     *  ordinary FNpcReaction threat read still applies). Matches FNpcReaction::NoticeRadius. */
    static constexpr double FleeRangeM = 14.0;
    /** At/within this range (m) a levelled weapon is point-blank — a scared citizen freezes. */
    static constexpr double CowerRangeM = 4.0;
    /** Bravery below this cowers at point-blank; at/above it the citizen still bolts. */
    static constexpr double CowerNerve = 0.5;

    /**
     * The duress response: None if the player isn't brandishing or is out of range;
     * Cower if a weapon is point-blank (<= CowerRangeM) and the citizen's nerve is
     * below CowerNerve; otherwise Flee.
     */
    static ENpcDuress Decide(double DistanceM, bool bArmed, double Bravery);
};
