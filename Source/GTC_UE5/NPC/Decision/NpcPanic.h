// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * How panic spreads person-to-person, not just from the event. FNpcWitness handles
 * the PRIMARY ripple — how vividly a bystander registers the gunshot/assault they
 * directly saw. This is the SECONDARY ripple: a citizen who is already terrified is
 * himself a source of fear, so the people around HIM flinch too, and the people
 * around THEM, until a single shot has emptied a whole plaza in a believable wave
 * that decays at the edges and dies out among the steady-nerved.
 *
 * FNpcReaction::CatchesPanic answers this as a binary (does the neighbour catch it,
 * yes/no, within a fixed radius). That can't form a gradient: panic either jumps at
 * full strength or not at all, so it can't fade across hops or pool thickest at the
 * epicentre. FNpcPanic is its graded successor — it returns HOW MUCH fear a neighbour
 * catches (0..1), scaled by how panicked the source is, how close they are, and the
 * neighbour's own nerve. Feed the caught fear back through Reinforce to let a crowd's
 * dread build toward (but never past) total panic with diminishing returns.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling — the
 * crowd director (UGTCCrowdSubsystem) does the spatial neighbour query and the
 * hop-by-hop propagation, feeding each pair's distance (metres, like FNpcMemory /
 * FNpcReaction / FNpcWitness; the adapter converts cm at the boundary), the source's
 * current fear, and the listener's bravery. Unit-tested headless
 * (Tests/NpcPanicTest.cpp, prefix GTC.NPC.Decision.NpcPanic).
 */
struct GTC_UE5_API FNpcPanic
{
    /** Fear spreads person-to-person within this range (metres). Matches
     *  FNpcReaction::PanicRadius so the graded model agrees with the binary one. */
    static constexpr double ContagionRadiusM = 6.0;

    /** Bravery at/above this shrugs off a neighbour's panic entirely — the steady ones
     *  hold the line and stop the wave. Matches FNpcReaction::PanicNerve. */
    static constexpr double SteadyNerve = 0.85;

    /** Below this share a neighbour is too far / too steady to catch anything — treated
     *  as not catching it. Matches FNpcWitness::MinShare. */
    static constexpr double MinShare = 0.05;

    /**
     * The fear (0..1) a neighbour catches from a panicked source.
     *   SourceFear — 0..1, how panicked the source citizen currently is
     *   DistanceM  — metres between source and neighbour
     *   Bravery    — 0..1 trait of the NEIGHBOUR; higher resists contagion
     *   RadiusM    — contagion range; linear falloff to 0 at the edge
     * Returns 0 beyond the radius, for an already-calm source, for a steady-nerved
     * neighbour (bravery >= SteadyNerve), or below MinShare. Result clamped 0..1.
     */
    static double CaughtFear(double SourceFear, double DistanceM, double Bravery,
        double RadiusM = ContagionRadiusM);

    /**
     * Would any fear spread at all (CaughtFear > 0)? The graded successor's stand-in
     * for FNpcReaction::CatchesPanic — but stricter: it also requires the source to
     * actually be afraid and the catch to clear MinShare, not merely proximity + nerve.
     */
    static bool WouldSpread(double SourceFear, double DistanceM, double Bravery,
        double RadiusM = ContagionRadiusM);

    /**
     * Combine a neighbour's CurrentFear with freshly CaughtFear so dread accumulates
     * across multiple panicked neighbours yet saturates — it builds toward 1 with
     * diminishing returns and never overshoots (probabilistic-OR shape:
     * current + caught*(1 - current)). Both inputs clamped 0..1.
     */
    static double Reinforce(double CurrentFear, double CaughtFear);
};
