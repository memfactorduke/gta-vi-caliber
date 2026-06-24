// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure decision: should a responding officer try to ARREST the player — close to
 * contact and cuff — rather than shoot it out? This is what finally lets the
 * already-wired bust loop (FArrestModel / UArrestSubsystem) actually land: the
 * gunfight AI (FPoliceCombat) holds officers at the ~12-20 m engagement band and
 * never approaches the 2 m catch range, so without an apprehend behaviour a bust
 * can only happen if the player walks into a cop.
 *
 * The rule mirrors a real low-heat police response: at one or two stars, against
 * an unarmed, non-threatening suspect, cops move in to arrest; the moment the
 * suspect is armed, dangerous, or the heat escalates, they revert to the lethal
 * gunfight AI. All-static, scene-free, deterministic, unit-tested
 * (Tests/ApprehendTest.cpp, prefix GTC.AI.Combat.Apprehend). The owning officer
 * node reads bPlayerArmed / threat from the crowd threat snapshot and drives the
 * approach; "planar" is the XZ plane with Y ignored, matching the rest of the
 * combat core (NO Z-up remap).
 */
class GTC_UE5_API FApprehend
{
public:
    /** Above this star count the response is lethal — no cuffing, shoot it out. */
    static constexpr int32 MaxApprehendStars = 2;

    /** Player "threat" (0..1) at/above which even a low-heat cop won't close to cuff. */
    static constexpr double ThreatCeiling = 0.5;

    /**
     * Should this officer move in to arrest rather than engage? True only when the
     * player is wanted at low heat (1..MaxApprehendStars), unarmed, and not reading
     * as a lethal threat. Anything else (no stars, high heat, armed, or dangerous)
     * returns false and the officer falls back to the FPoliceCombat gunfight plan.
     */
    static bool ShouldApprehend(int32 Stars, bool bPlayerArmed, double PlayerThreat01)
    {
        if (Stars <= 0 || Stars > MaxApprehendStars)
        {
            return false;
        }
        if (bPlayerArmed)
        {
            return false;
        }
        return FMath::Clamp(PlayerThreat01, 0.0, 1.0) < ThreatCeiling;
    }

    /** True once close enough to begin cuffing — the officer plants and the bust
     *  loop's grapple timer (FArrestModel) takes over. */
    static bool InCatchRange(double DistanceM, double CatchRangeM)
    {
        return DistanceM <= FMath::Max(CatchRangeM, 0.0);
    }

    /** Planar unit heading to close on the target, or ZeroVector if coincident. */
    static FVector ApproachHeading(const FVector& SelfPos, const FVector& TargetPos)
    {
        const FVector D(TargetPos.X - SelfPos.X, 0.0, TargetPos.Z - SelfPos.Z);
        return D.Size() > 0.0001 ? D.GetSafeNormal() : FVector::ZeroVector;
    }

    /**
     * Move speed while closing to cuff: a full-run approach until inside the catch
     * range, then zero so the officer plants to make the arrest (no overrun).
     */
    static double ApproachSpeed(double RunSpeed, double DistanceM, double CatchRangeM)
    {
        return InCatchRange(DistanceM, CatchRangeM) ? 0.0 : FMath::Max(RunSpeed, 0.0);
    }
};
