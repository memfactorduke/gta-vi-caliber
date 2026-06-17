// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure satisfaction/progress evaluators for the varied open-world-style objective KINDS
 * (reach a point, collect N items, eliminate targets, escort an NPC, survive a
 * timer, defend a structure) so missions are not all "drive to a zone".
 *
 * Godot parity: game/scripts/missions/mission_objective_types.gd (RefCounted,
 * all-static helpers plus a nested Counter class). This layer answers, per
 * objective, "is it satisfied / failed / how far along (0..1)". Static, scene-free,
 * RNG-free, defensively clamped — unit-tested headless. The nested Counter is the
 * only stateful piece, a small tally for COLLECT / wave objectives.
 *
 * Counter mirrors Godot's MissionObjectiveTypes.Counter (a nested class) as a
 * nested C++ type.
 */
class GTC_UE5_API MissionObjectiveTypes
{
public:
    enum class EKind : uint8
    {
        Reach,
        Collect,
        Eliminate,
        Escort,
        Survive,
        Defend
    };

    /** Stable string id for a kind (HUD/save use), or "" for an unknown value. */
    static FString KindName(int32 Kind);

    /** REACH: player is within Radius of the target point (radius floored at 0). */
    static bool ReachSatisfied(const FVector& PlayerPos, const FVector& TargetPos, double Radius);

    /**
     * COLLECT: fraction gathered in [0,1]. A required count <= 0 is "nothing to do",
     * so it reports fully complete (1.0); over-collecting caps at 1.0.
     */
    static double CollectProgress(int32 Collected, int32 Required);

    /** COLLECT done: gathered at least Required (a required <= 0 is instantly done). */
    static bool CollectSatisfied(int32 Collected, int32 Required);

    /** ELIMINATE done: no targets left to kill (negative treated as 0). */
    static bool EliminateSatisfied(int32 TargetsRemaining);

    /** ESCORT failed: the escortee died (health at or below 0). */
    static bool EscortFailed(double EscorteeHealth);

    /** ESCORT done: the escortee reached the drop-off within Radius. */
    static bool EscortSatisfied(const FVector& EscorteePos, const FVector& DestPos, double Radius);

    /**
     * SURVIVE: fraction of the hold-out timer elapsed in [0,1]. A duration <= 0 is a
     * degenerate "no wait", reported as complete (1.0).
     */
    static double SurviveProgress(double TimeSurvived, double Duration);

    /** SURVIVE done: held out for the full duration (a duration <= 0 is instantly done). */
    static bool SurviveSatisfied(double TimeSurvived, double Duration);

    /** DEFEND failed: the protected structure was destroyed (health at or below 0). */
    static bool DefendFailed(double StructureHealth);

    /**
     * Small mutable tally for COLLECT / wave-clear objectives: count up toward a
     * fixed target, ignoring negative deltas and capping reported progress at done.
     */
    class GTC_UE5_API Counter
    {
    public:
        /** target is floored at 0. */
        explicit Counter(int32 Target = 0);

        /** Add N to the tally (negative or zero is ignored); returns the new count. */
        int32 Add(int32 N);

        int32 Count() const;
        int32 Target() const;

        /** Items still needed to finish, never below 0 (0 once the target is met). */
        int32 Remaining() const;

        /** True once the target is reached (a target of 0 is done immediately). */
        bool IsDone() const;

        /** Tally fraction in [0,1]; a target of 0 reports 1.0, over-count caps at 1.0. */
        double Progress() const;

        /** Wipe the tally back to empty (keeps the target) for a mission retry. */
        void Reset();

    private:
        int32 TargetCount = 0;
        int32 CurrentCount = 0;
    };
};
