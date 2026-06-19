// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure combat decision AI: turns a pursuer's situation into a discrete combat
 * action and a movement intent. This is the connective tissue between
 * FPoliceResponse::Aggression(stars) (how hard the law leans on the player) and
 * an actual shootout — when to close distance, when to open fire, when to break
 * the line of sight, when to take cover.
 *
 * Direct port of the Godot `CombatAi` (RefCounted) at
 * `game/scripts/ai/combat_ai.gd`. All-static plain functions, no UObject, no
 * scene access, no RNG, no node state — so behaviour is deterministic and
 * unit-tested headless (Tests/CombatAiTest.cpp, prefix GTC.AI.Combat.CombatAi).
 * The owning enemy/officer node holds mutable state (current cover point, fire
 * cooldown, strafe side) and feeds these helpers each tick. "Planar" means the
 * XZ plane with Y ignored, matching the Godot source — NO Z-up remap.
 *
 * PURE-CORE boundary: the helpers take distance/positions/flags as plain inputs.
 * The EQS cover query, Behavior Tree wiring, and line-of-fire/LOS traces that
 * feed `los_clear`/`in_arc` are the DEFERRED Wave-3 adapter — NOT implemented
 * here and NOT covered by the parity tests. Computed precision is `double` to
 * match the GDScript oracle's float arithmetic.
 */

/**
 * Discrete combat actions. Godot enum ordinal order preserved.
 */
enum class ECombatAction : uint8
{
    Advance,     // close toward the target (out of range, or no line of sight)
    Engage,      // in band, aimed, sightline clear -> shoot
    Reposition,  // in range but too close / not aimed -> strafe to a better angle
    TakeCover,   // hurt and not fully committed -> seek cover
    Retreat,     // overwhelmed or out of ammo with low resolve -> fall back
};

class GTC_UE5_API FCombatAi
{
public:
    /** Below this health fraction a unit seeks cover unless its aggression is lethal. */
    static constexpr double CoverHealth = 0.35;
    /** Below this health fraction a low-aggression unit breaks off entirely. */
    static constexpr double RetreatHealth = 0.15;
    /**
     * Default half-angle (radians) of the firing arc — the target must be roughly
     * in front before a unit will shoot rather than turn/strafe to face it.
     */
    static constexpr double DefaultArcHalf = UE_DOUBLE_PI * 35.0 / 180.0;

    /**
     * The engagement distance band around a weapon's preferred range, returned as
     * (X = inner, Y = outer). Units advance when beyond Y, back off when inside X,
     * and hold-and-fire in between. The hysteresis gap keeps a unit from
     * flip-flopping between Advance and Reposition at a single threshold.
     */
    static FVector2D EngagementBand(double PreferredRange, double Hysteresis)
    {
        const double H = FMath::Clamp(Hysteresis, 0.0, 0.9);
        return FVector2D(PreferredRange * (1.0 - H), PreferredRange * (1.0 + H));
    }

    /** Planar unit direction from A to B, or ZeroVector if effectively coincident. */
    static FVector PlanarDir(const FVector& A, const FVector& B)
    {
        const FVector D(B.X - A.X, 0.0, B.Z - A.Z);
        return D.Size() > 0.0001 ? D.GetSafeNormal() : FVector::ZeroVector;
    }

    /**
     * True if `TargetDir` falls within `HalfAngle` radians of where the unit
     * faces. Both vectors are treated as planar unit directions.
     */
    static bool InFiringArc(const FVector& Facing, const FVector& TargetDir, double HalfAngle)
    {
        const FVector F(Facing.X, 0.0, Facing.Z);
        const FVector T(TargetDir.X, 0.0, TargetDir.Z);
        if (F.Size() < 0.0001 || T.Size() < 0.0001)
        {
            return false;
        }
        return F.GetSafeNormal().Dot(T.GetSafeNormal()) >= FMath::Cos(FMath::Clamp(HalfAngle, 0.0, UE_DOUBLE_PI));
    }

    /**
     * The core decision. Given the tactical picture, pick one action. Order
     * matters: survival (ammo, cover, retreat) is checked before sightline,
     * range, and aim so a unit never charges into fire it can't return.
     */
    static ECombatAction DecideAction(
        double Distance,
        const FVector2D& Band,
        bool bLosClear,
        bool bInArc,
        double HealthFrac,
        double Aggression,
        int32 Ammo)
    {
        // No ammo: lethal units strafe to reload under pressure; the rest fall back.
        if (Ammo <= 0)
        {
            return Aggression >= 0.8 ? ECombatAction::Reposition : ECombatAction::Retreat;
        }
        // Badly hurt and not committed -> break off.
        if (HealthFrac <= RetreatHealth && Aggression < 0.5)
        {
            return ECombatAction::Retreat;
        }
        // Hurt but still in the fight -> use cover, unless aggression is maxed.
        if (HealthFrac <= CoverHealth && Aggression < 1.0)
        {
            return ECombatAction::TakeCover;
        }
        // Can't see the target -> move to flank/regain line of sight.
        if (!bLosClear)
        {
            return ECombatAction::Advance;
        }
        if (Distance > Band.Y)
        {
            return ECombatAction::Advance;
        }
        if (Distance < Band.X)
        {
            return ECombatAction::Reposition;
        }
        // In band with a clear shot: fire if aimed, otherwise adjust angle.
        return bInArc ? ECombatAction::Engage : ECombatAction::Reposition;
    }

    /**
     * Whether to actually pull the trigger this tick: only while Engage-ing and
     * the per-unit fire cooldown has elapsed. Keeps the trigger gate in one place.
     */
    static bool ShouldFire(ECombatAction Action, bool bCooldownReady)
    {
        return Action == ECombatAction::Engage && bCooldownReady;
    }

    /**
     * Seconds between shots, scaled by aggression so high-heat responders fire
     * faster. aggression 0 -> 1.8x the base interval (cautious), 1 -> 0.6x
     * (relentless).
     */
    static double FireInterval(double BaseInterval, double Aggression)
    {
        return BaseInterval * FMath::Lerp(1.8, 0.6, FMath::Clamp(Aggression, 0.0, 1.0));
    }

    /**
     * Planar unit movement intent for an action. TakeCover and Retreat both run
     * away from the target as a fallback — the owning node overrides TakeCover
     * with a real cover point when it has one. Engage holds position (ZeroVector).
     * `StrafeSign` (+1/-1) is the unit's stable circling side, chosen by the node.
     */
    static FVector DesiredMove(
        ECombatAction Action, const FVector& SelfPos, const FVector& TargetPos, double StrafeSign)
    {
        const FVector ToTarget = PlanarDir(SelfPos, TargetPos);
        switch (Action)
        {
            case ECombatAction::Advance:
                return ToTarget;
            case ECombatAction::Retreat:
            case ECombatAction::TakeCover:
                return -ToTarget;
            case ECombatAction::Reposition:
            {
                // Perpendicular to the sightline, picked side; bias slightly
                // outward so "too close" repositioning also opens distance.
                const FVector Perp = FVector(ToTarget.Z, 0.0, -ToTarget.X) * FMath::Sign(StrafeSign);
                return (Perp - ToTarget * 0.3).GetSafeNormal();
            }
            default:  // Engage
                return FVector::ZeroVector;
        }
    }

    /**
     * Target move speed for an action. Engage plants the feet to fire; strafing
     * is a touch slower than an all-out advance/retreat.
     */
    static double MoveSpeed(ECombatAction Action, double RunSpeed)
    {
        switch (Action)
        {
            case ECombatAction::Advance:
            case ECombatAction::Retreat:
            case ECombatAction::TakeCover:
                return RunSpeed;
            case ECombatAction::Reposition:
                return RunSpeed * 0.7;
            default:  // Engage
                return 0.0;
        }
    }
};
