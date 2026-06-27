// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Junction traffic lights — the signalized half of M4's intersection work
 * (the explicit "junction traffic lights" TODO in docs/ROADMAP.md). FIntersection
 * answers right-of-way at an UNSIGNALIZED junction (stop signs, all-way-stop,
 * priority road); FTrafficSignal answers the other kind: at a signalized junction,
 * what does the light show each approach RIGHT NOW. The two are siblings — the
 * adapter consults one or the other per junction, and neither knows about the
 * other.
 *
 * The model is a fixed phase cycle on a continuous clock, built so the city's
 * lights feel alive rather than scripted. A junction is configured with an ordered
 * ring of phases; each phase gives GREEN then YELLOW to a set of approach groups
 * (e.g. north+south together) while every other group holds RED, and an optional
 * all-red clearance separates one phase from the next so cross traffic has emptied
 * the box before the next phase goes green. The whole ring repeats forever. Like
 * the radio dial, the controller advances ONE shared clock and derives the live
 * state from it, and a per-junction Offset slides each junction to a different
 * point in its cycle — so neighbouring intersections desync (and an adapter can
 * tune offsets into a green wave down an avenue) instead of blinking in unison.
 *
 * An approach group is just an integer id the adapter assigns to a set of lanes
 * that move together; a group never named in any phase simply stays RED forever
 * (the adapter reads a permanently-red group as "fall back to unsignalized
 * yielding"). TimeUntilChange lets an agent anticipate a yellow about to drop to
 * red (stop) or a red about to go green (creep), and feeds a HUD countdown.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The clock is the single source of truth, so the same elapsed time always yields
 * the same lights — unit-tested headless (Tests/TrafficSignalTest.cpp, prefix
 * GTC.AI.Traffic.Signal).
 *
 * PURE-CORE boundary: mapping a group's RED/YELLOW to a braked "stopped leader"
 * gap via FIntersection::StopLineGap each tick, driving the light mesh emissive,
 * and gathering which lanes belong to which group is the DEFERRED adapter and is
 * NOT covered by the tests. Units: every duration and the clock are in seconds.
 */
struct GTC_UE5_API FTrafficSignal
{
    /** What a single approach group's light shows this instant. */
    enum class ESignal : uint8
    {
        Red,    // hold at the stop line
        Yellow, // clearance — stop if you safely can, the phase is ending
        Green,  // go
    };

    /** One stage of the cycle: the groups that get GREEN→YELLOW while all others hold RED. */
    struct FPhase
    {
        /** Approach-group ids that move during this phase (Green then Yellow). Groups not
         *  listed here are Red for the whole phase. An empty set is a legal "all idle" stage. */
        TArray<int32> GoGroups;

        /** Seconds the GoGroups stay Green. Non-positive collapses the green to nothing. */
        double GreenSeconds = 0.0;

        /** Seconds the GoGroups stay Yellow after the green, before the all-red clearance.
         *  Non-positive collapses the yellow to nothing. */
        double YellowSeconds = 0.0;
    };

    /**
     * Install the cycle and reset the clock to 0. `InAllRedSeconds` is the all-red
     * clearance inserted AFTER each phase (every approach Red); non-positive means
     * no clearance. `InOffsetSeconds` slides this junction along its own cycle so
     * neighbours desync; it is folded modulo the cycle just like the clock and may
     * be negative. Phases are taken by value so the caller's array can be moved in.
     */
    void Configure(TArray<FPhase> InPhases, double InAllRedSeconds = 0.0, double InOffsetSeconds = 0.0);

    /**
     * Advance the shared cycle clock by Dt seconds. Call once per frame; a negative
     * Dt is ignored so a paused/rewound game can't run the lights backwards.
     */
    void Advance(double Dt);

    /** Seconds elapsed on the shared clock since Configure. */
    double Clock() const { return ClockSeconds; }

    /** Total length of one full cycle (all phases' green+yellow plus one all-red each), in seconds. */
    double CycleSeconds() const { return TotalCycle; }

    /**
     * The light shown to approach-group `Group` right now. A group not named in the
     * active phase — or any group during an all-red clearance, or when the cycle has
     * no length — is Red.
     */
    ESignal StateFor(int32 Group) const;

    /**
     * Index of the phase currently running (its green or yellow, OR the all-red
     * clearance that follows it). INDEX_NONE when the cycle has no length.
     */
    int32 ActivePhase() const;

    /** True while the controller is in the all-red clearance after the active phase. */
    bool IsAllRed() const;

    /**
     * Seconds until the active group-states next change — i.e. until green→yellow,
     * yellow→all-red (or →next green), or the all-red→next green boundary. 0 when the
     * cycle has no length. Lets an agent anticipate the change and a HUD count down.
     */
    double TimeUntilChange() const;

private:
    // Resolve the clock to (phase index, segment, seconds remaining in this segment).
    enum class ESegment : uint8 { Green, Yellow, AllRed };
    struct FResolved
    {
        int32 Phase = INDEX_NONE;
        ESegment Segment = ESegment::AllRed;
        double Remaining = 0.0;
        bool bValid = false;
    };
    FResolved Resolve() const;

    TArray<FPhase> Phases;
    double AllRedSeconds = 0.0;
    double OffsetSeconds = 0.0;
    double ClockSeconds = 0.0;
    double TotalCycle = 0.0;
};
