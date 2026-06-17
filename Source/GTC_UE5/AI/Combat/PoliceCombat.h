// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "CombatAi.h"
#include "../../World/Police/PoliceResponse.h"

/**
 * Pure composition layer that turns wanted-level heat into a police gunfight.
 *
 * It wires together two existing pure models so the Police node has ONE call and
 * the composition itself is unit-tested (Tests/PoliceCombatTest.cpp, prefix
 * GTC.AI.Combat.PoliceCombat):
 *   - FCombatAi       — the tactical decision brain (advance/engage/reposition/…)
 *   - FPoliceResponse — maps wanted stars -> aggression (fire rate, chase speed)
 *
 * Direct port of the the reference `PoliceCombat` (RefCounted) at
 * `game/scripts/ai/police_combat.gd`. All-static plain functions, no UObject, no
 * scene/RNG/node state, "planar" = XZ with Y ignored — same convention as the
 * the reference source (NO Z-up remap). The Police node owns the mutable state (fire
 * cooldown, ammo, aim heading) and just executes the plan.
 *
 * PURE-CORE boundary: `los_clear`/`to_target_dir` come from Wave-3 LOS/aim
 * adapters NOT implemented or tested here. Computed precision is `double` to
 * match the the reference implementation oracle.
 */

class GTC_UE5_API FPoliceCombat
{
public:
    /** Seconds between shots at neutral aggression, before heat scaling. */
    static constexpr double BaseFireInterval = 1.1;
    /** A pistol's comfortable engagement distance — cops hold here and fire. */
    static constexpr double PreferredRange = 16.0;
    /** Half-width of the engagement band as a fraction of PreferredRange. */
    static constexpr double BandHysteresis = 0.28;
    /** Chase speed multipliers at minimum (calm) and maximum (lethal) aggression. */
    static constexpr double ChaseSpeedMinScale = 0.85;
    static constexpr double ChaseSpeedMaxScale = 1.25;

    /**
     * The whole per-tick decision for one officer, as a small deterministic
     * record (the the reference `plan()` Dictionary as a nested value type):
     *   Action     — FCombatAi action to execute this tick
     *   bFire      — true iff the officer should pull the trigger now
     *   bInArc     — whether the target is within the firing arc (debug/feel)
     *   Aggression — the heat-derived aggression in [0, 1]
     */
    struct FCombatPlan
    {
        ECombatAction Action = ECombatAction::Advance;
        bool bFire = false;
        bool bInArc = false;
        double Aggression = 0.0;
    };

    /** The engagement distance band cops try to hold around PreferredRange. */
    static FVector2D Band()
    {
        return FCombatAi::EngagementBand(PreferredRange, BandHysteresis);
    }

    /**
     * The whole per-tick decision for one officer. `Facing` and `ToTargetDir` are
     * planar unit headings; the node supplies its current aim heading and the
     * direction to the player. `bCooldownReady` is true once the per-officer fire
     * timer has elapsed.
     */
    static FCombatPlan Plan(
        double Distance,
        bool bLosClear,
        const FVector& Facing,
        const FVector& ToTargetDir,
        double HealthFrac,
        int32 Stars,
        int32 Ammo,
        bool bCooldownReady)
    {
        const double Aggression = FPoliceResponse::Aggression(Stars);
        const bool bInArc = FCombatAi::InFiringArc(Facing, ToTargetDir, FCombatAi::DefaultArcHalf);
        const ECombatAction Action = FCombatAi::DecideAction(
            Distance, Band(), bLosClear, bInArc, HealthFrac, Aggression, Ammo);

        FCombatPlan P;
        P.Action = Action;
        P.bFire = FCombatAi::ShouldFire(Action, bCooldownReady);
        P.bInArc = bInArc;
        P.Aggression = Aggression;
        return P;
    }

    /**
     * Seconds to wait before the next shot, scaled by heat — higher wanted levels
     * make responders fire faster (FCombatAi::FireInterval shrinks with
     * aggression).
     */
    static double FireCooldown(int32 Stars)
    {
        return FCombatAi::FireInterval(BaseFireInterval, FPoliceResponse::Aggression(Stars));
    }

    /**
     * Chase speed for the given base run speed, scaled by heat so a 5-star
     * response closes faster than a 1-star one.
     */
    static double ChaseSpeed(double BaseRun, int32 Stars)
    {
        return BaseRun
            * FMath::Lerp(ChaseSpeedMinScale, ChaseSpeedMaxScale, FPoliceResponse::Aggression(Stars));
    }
};
