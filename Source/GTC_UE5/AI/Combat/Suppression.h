// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure suppression model: sustained incoming fire PINS an officer down even before
 * it kills them. Suppression builds with each round that lands, decays once the
 * shooting lets up, and while it is high the officer fires less and clings to cover
 * — which is what makes "suppressing fire" a real tactic rather than just raw
 * damage. The owning officer node holds the live level and feeds these helpers.
 *
 * All-static, scene-free, deterministic, double precision, no UObject/RNG — so the
 * curve is unit-tested headless (Tests/SuppressionTest.cpp, prefix
 * GTC.AI.Combat.Suppression).
 */
class GTC_UE5_API FSuppression
{
public:
    /** At/above this level the officer reads as suppressed (cover + slower fire). */
    static constexpr double SuppressedThreshold = 0.5;

    /** How fast suppression bleeds off per second once the rounds stop landing. */
    static constexpr double DefaultDecayPerSec = 0.35;

    /**
     * Suppression a single landed round adds: a base jolt plus more for a harder
     * hit (WoundSeverity 0..1). Even a graze pins you a little; a heavy hit a lot.
     */
    static double FromHit(double WoundSeverity)
    {
        return FMath::Clamp(0.25 + 0.35 * FMath::Clamp(WoundSeverity, 0.0, 1.0), 0.0, 1.0);
    }

    /** Accumulate suppression, clamped to [0, 1]. */
    static double Add(double Current, double Amount)
    {
        return FMath::Clamp(Current + Amount, 0.0, 1.0);
    }

    /** Bleed suppression off over `Dt` seconds (floored at 0). */
    static double Decay(double Current, double Dt, double Rate = DefaultDecayPerSec)
    {
        return FMath::Max(0.0, Current - FMath::Max(Rate, 0.0) * FMath::Max(Dt, 0.0));
    }

    /** Whether the level is high enough to change behaviour. */
    static bool IsSuppressed(double Level, double Threshold = SuppressedThreshold)
    {
        return Level >= Threshold;
    }

    /**
     * Fire-interval stretch while suppressed: 1.0 (no effect) at zero, up to ~2.2x
     * the base interval at full suppression — a pinned officer shoots less often.
     */
    static double FireRateMult(double Level)
    {
        return 1.0 + 1.2 * FMath::Clamp(Level, 0.0, 1.0);
    }

    /**
     * Effective "health" the tactical AI should plan against — real health scaled
     * DOWN by suppression, so a heavily-suppressed officer seeks cover sooner even
     * when not badly hurt. 0.5x weight: full suppression halves perceived health.
     */
    static double EffectiveHealthFrac(double HealthFrac, double Level)
    {
        return FMath::Clamp(HealthFrac * (1.0 - 0.5 * FMath::Clamp(Level, 0.0, 1.0)), 0.0, 1.0);
    }
};
