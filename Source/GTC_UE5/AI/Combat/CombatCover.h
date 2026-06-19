// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure cover evaluation for shootouts — the spatial reasoning that lets the
 * player and the AI duck behind walls, lean out to shoot, and choose the safest
 * spot relative to a threat. Static math only, no nodes, no RNG, so behaviour is
 * deterministic and unit-tested headless (Tests/CombatCoverTest.cpp, prefix
 * GTC.AI.Combat.CombatCover).
 *
 * Direct port of the the reference `CombatCover` (RefCounted) at
 * `game/scripts/ai/combat_cover.gd`. The the reference cover point is a Dictionary
 * {pos: Vector3, normal: Vector3}; here it is the plain `FCoverPoint` value type
 * (nested the reference Dictionary -> nested C++ struct). `Normal` is the direction the
 * cover FACES — its open side, the side an agent stands on to be protected.
 * Convention: a threat is blocked when it sits on the faced side, i.e.
 * dot(threat - pos, normal) > 0 (the wall is between the agent and the threat).
 *
 * Planar throughout: the XZ plane with Y ignored (Y is up), matching the Godot
 * source — NO Z-up remap. The owning node holds mutable state (chosen cover,
 * peek side); these helpers just score and locate.
 *
 * PURE-CORE boundary: best-cover scoring runs over caller-supplied candidate
 * positions. The EQS query that produces those candidates and the physics LOS
 * traces are the DEFERRED Wave-3 adapter — NOT implemented/tested here. Computed
 * precision is `double` to match the the reference implementation oracle.
 */

class GTC_UE5_API FCombatCover
{
public:
    /**
     * Below this planar distance a cover and a threat are treated as coincident,
     * so we never normalise a zero vector into a NaN.
     */
    static constexpr double Epsilon = 0.0001;

    /**
     * A cover point — the Godot {pos, normal} Dictionary as a plain value type.
     * An empty/unset cover is represented by FCoverPoint{} (zero pos and normal);
     * BestCover returns bValid == false to mark "no cover found".
     */
    struct FCoverPoint
    {
        FVector Pos = FVector::ZeroVector;
        FVector Normal = FVector::ZeroVector;
        /** False for the "no protecting cover" sentinel returned by BestCover. */
        bool bValid = true;
    };

    /** Planar (XZ) vector with the vertical component dropped. */
    static FVector Planar(const FVector& V)
    {
        return FVector(V.X, 0.0, V.Z);
    }

    /** Planar unit direction from A to B, or ZeroVector if effectively coincident. */
    static FVector PlanarDir(const FVector& A, const FVector& B)
    {
        const FVector D = Planar(B - A);
        return D.Size() > Epsilon ? D.GetSafeNormal() : FVector::ZeroVector;
    }

    /**
     * Normalized horizontal direction from the agent toward the threat — what to
     * face when aiming. ZeroVector if they're on top of each other.
     */
    static FVector ThreatDirection(const FVector& AgentPos, const FVector& ThreatPos)
    {
        return PlanarDir(AgentPos, ThreatPos);
    }

    /**
     * True if this cover protects against a threat at `ThreatPos`: the threat
     * must be on the side the cover faces, so the cover body sits between agent
     * and threat. dot(threat - pos, normal) > 0 => protected. A zero/degenerate
     * normal can't block anything, so returns false.
     */
    static bool ProvidesCover(const FCoverPoint& Cover, const FVector& ThreatPos)
    {
        const FVector Normal = Planar(Cover.Normal);
        if (Normal.Size() < Epsilon)
        {
            return false;
        }
        const FVector ToThreat = Planar(ThreatPos - Cover.Pos);
        if (ToThreat.Size() < Epsilon)
        {
            return false;
        }
        return Normal.GetSafeNormal().Dot(ToThreat.GetSafeNormal()) > 0.0;
    }

    /**
     * How good this cover is against the threat, in [0, 1]. Best when the cover
     * squarely faces the threat (normal aligned with the cover->threat line) AND
     * the threat is far enough that the wall actually shields a meaningful arc.
     * Zero when it doesn't protect (threat on the open side) or the normal is
     * degenerate. `AgentToProtectRadius` widens the "too close to matter" band: a
     * bigger body needs the threat further off before the cover counts as
     * squarely facing.
     */
    static double CoverQuality(
        const FCoverPoint& Cover, const FVector& ThreatPos, double AgentToProtectRadius)
    {
        const FVector Normal = Planar(Cover.Normal);
        if (Normal.Size() < Epsilon)
        {
            return 0.0;
        }
        const FVector ToThreat = Planar(ThreatPos - Cover.Pos);
        const double Dist = ToThreat.Size();
        if (Dist < Epsilon)
        {
            return 0.0;
        }
        const double Facing = Normal.GetSafeNormal().Dot(ToThreat / Dist);
        if (Facing <= 0.0)
        {
            return 0.0;
        }
        // Range factor: the threat must clear the agent's own radius before the
        // wall shields a useful arc; saturates to 1 once comfortably beyond it.
        const double MinUseful = FMath::Max(AgentToProtectRadius, Epsilon);
        const double RangeFactor = FMath::Clamp((Dist - MinUseful) / (MinUseful + 1.0), 0.0, 1.0);
        return FMath::Clamp(Facing * RangeFactor, 0.0, 1.0);
    }

    /**
     * Pick the cover that protects the agent from the threat and is nearest the
     * agent. Returns the chosen cover, or FCoverPoint{ bValid = false } if the
     * list is empty or none protect.
     */
    static FCoverPoint BestCover(
        const TArray<FCoverPoint>& CoverPoints, const FVector& AgentPos, const FVector& ThreatPos)
    {
        FCoverPoint Best;
        Best.bValid = false;
        double BestDist = TNumericLimits<double>::Max();
        for (const FCoverPoint& Cover : CoverPoints)
        {
            if (!ProvidesCover(Cover, ThreatPos))
            {
                continue;
            }
            const double D = Planar(Cover.Pos - AgentPos).Size();
            if (D < BestDist)
            {
                BestDist = D;
                Best = Cover;
                Best.bValid = true;
            }
        }
        return Best;
    }

    /**
     * A spot stepped sideways along the cover face from which the agent can see
     * and shoot the threat. Perpendicular to the cover->threat line, so leaning
     * out clears the wall edge rather than walking into the line of fire.
     * `PeekOffset` is how far to lean (negative flips to the other edge). Falls
     * back to the cover position when the threat is coincident (no defined
     * sideways).
     */
    static FVector PeekPosition(const FCoverPoint& Cover, const FVector& ThreatPos, double PeekOffset)
    {
        const FVector Pos = Cover.Pos;
        const FVector ToThreat = PlanarDir(Pos, ThreatPos);
        if (ToThreat.Size() < Epsilon)
        {
            return Pos;
        }
        // Right-hand perpendicular in XZ to the cover->threat line.
        const FVector Side(ToThreat.Z, 0.0, -ToThreat.X);
        return Pos + Side * PeekOffset;
    }

    /**
     * True if the agent has stepped out of cover into the threat's sightline —
     * i.e. the agent is now on the threat's side of the wall (the open side)
     * rather than tucked behind it. When the cover can't protect at all
     * (degenerate normal / threat on the open side), any position counts as
     * exposed.
     */
    static bool IsExposed(const FVector& AgentPos, const FCoverPoint& Cover, const FVector& ThreatPos)
    {
        if (!ProvidesCover(Cover, ThreatPos))
        {
            return true;
        }
        const FVector Normal = Planar(Cover.Normal).GetSafeNormal();
        const FVector Pos = Cover.Pos;
        const FVector AgentOffset = Planar(AgentPos - Pos);
        // Agent is exposed once it crosses the wall plane onto the threat-facing side.
        return AgentOffset.Dot(Normal) > 0.0;
    }
};
