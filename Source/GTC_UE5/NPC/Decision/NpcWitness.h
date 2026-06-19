// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * How violence ripples through a crowd. When the player assaults a citizen, the
 * people who SEE it should react too — not just the victim. This is the pure
 * falloff math for "how vividly does a bystander at distance D register what just
 * happened": close witnesses are alarmed (they'll recognise and flee the player
 * via FNpcMemory), distant ones barely notice. It turns one shove into a street
 * that flinches, backs away, and remembers — the difference between hitting a prop
 * and hitting a person in a living city.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling — the
 * crowd director (UGTCCrowdSubsystem) does the spatial broadcast and feeds each
 * witness's share into AGTCCitizen::WitnessPlayerMayhem (which the existing
 * FNpcMemory recognition/threat machinery already acts on). Unit-tested headless
 * (Tests/NpcWitnessTest.cpp, prefix GTC.NPC.Decision.NpcWitness). Distances are in
 * metres, matching FNpcMemory / FNpcReaction; the adapter converts cm at the
 * boundary.
 */
struct GTC_UE5_API FNpcWitness
{
    /** Bystanders within this range (metres) can witness an assault. Matches the
     *  FNpcMemory recognition range, so a witness who registers it can also
     *  recognise the culprit while the memory lasts. */
    static constexpr double DefaultRadiusM = 12.0;

    /** Below this share a witness is too far to care — treated as not noticing. */
    static constexpr double MinShare = 0.05;

    /**
     * The memory intensity a bystander at DistanceM picks up from a mayhem event of
     * BaseIntensity (0..1, e.g. how brutal the hit was), with linear falloff to 0 at
     * RadiusM. Returns 0 beyond the radius or below MinShare. Result clamped 0..1.
     */
    static double Share(double BaseIntensity, double DistanceM, double RadiusM = DefaultRadiusM);
};
