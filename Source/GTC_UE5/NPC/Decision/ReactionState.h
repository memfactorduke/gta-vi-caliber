// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The stateful layer over FNpcReaction. FNpcReaction::Decide answers "ignore /
 * gawk / flee" fresh every tick from the player's current threat + distance — but
 * driven raw, a citizen flickers between verbs frame to frame as the player j{i}tters
 * at the edge of a radius. FReactionState turns that noisy per-tick verb into a
 * committed reaction with rising-edge triggers and hysteresis, exactly like the
 * repo's rising-edge bark/flinch (OnTrafficStartle): escalation is INSTANT and
 * fires a rising edge (the moment you play the gasp / break into a run), while
 * de-escalation is DELAYED by a per-state hold (a fleeing citizen keeps running a
 * beat after the gun is gone, then eases to a wary stare, then back to their day).
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject
 * — unit-tested headless (Tests/ReactionStateTest.cpp, prefix
 * GTC.NPC.Decision.ReactionState).
 *
 * The core is verb-string-free (enum in / enum out); the adapter maps
 * FNpcReaction::Decide's "ignore"/"gawk"/"flee" to EReaction at the boundary.
 * PURE-CORE boundary: gathering the threat snapshot and firing the
 * animation/audio on the rising edge is the DEFERRED adapter, NOT covered here.
 */
struct GTC_UE5_API FReactionState
{
    /** Escalation order: Flee outranks Gawk outranks Ignore. */
    enum class EReaction : uint8
    {
        Ignore = 0,
        Gawk = 1,
        Flee = 2,
    };

    /** A fleeing citizen holds the run at least this long after the threat clears. */
    static constexpr double FleeHoldSeconds = 3.0;
    /** A gawking citizen holds the stare at least this long before losing interest. */
    static constexpr double GawkHoldSeconds = 1.0;

    /** The result of a tick: the (possibly changed) state and whether it just escalated. */
    struct FTick
    {
        EReaction State = EReaction::Ignore;
        bool bRisingEdge = false; // true only on the tick the reaction INCREASES
    };

    /** Reaction rank (Ignore 0 .. Flee 2) for ordering. */
    static int32 Rank(EReaction R);
    /** One reaction below R (Flee->Gawk->Ignore; Ignore stays Ignore). */
    static EReaction Lower(EReaction R);
    /** Minimum seconds R must be held before it can step down. */
    static double HoldFor(EReaction R);

    EReaction Current() const { return State; }
    double TimeInState() const { return TimeInStateSec; }
    bool IsReacting() const { return State != EReaction::Ignore; }
    void Reset() { State = EReaction::Ignore; TimeInStateSec = 0.0; }

    /**
     * Advance the FSM by DeltaTime against this tick's Desired reaction (the mapped
     * FNpcReaction verb). Escalation to a higher reaction is immediate and reports
     * a rising edge; a drop to a lower reaction waits out HoldFor(current) and then
     * steps down one level (never below Desired), without a rising edge.
     */
    FTick Update(EReaction Desired, double DeltaTime);

private:
    EReaction State = EReaction::Ignore;
    double TimeInStateSec = 0.0;
};
