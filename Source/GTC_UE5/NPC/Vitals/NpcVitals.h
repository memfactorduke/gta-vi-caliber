// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The outcome of a single hit on a citizen's vitals, so the AGTCCitizen adapter
 * can branch the body's response in one switch: nothing, a wound reaction, or
 * death.
 */
enum class ENpcHitResult : uint8
{
    Ignored, // already down, or non-positive damage — nothing changed
    Wounded, // took damage and is still on their feet
    Killed,  // this hit dropped them to zero
};

/**
 * FNpcVitals — a citizen's life, as a plain value type.
 *
 * Deliberately simpler than FPlayerHealthModel: a pedestrian is disposable, so
 * there is no regeneration and no armor — just a health pool that a bullet
 * draws down until it hits zero. Toughness is the one knob (a cop seeded with a
 * bigger MaxHealth soaks more rounds than a tourist).
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine/UObject
 * coupling, so the damage math is unit-tested headless
 * (Tests/NpcVitalsTest.cpp, prefix GTC.NPC.Vitals). The acting-on-the-result
 * half — flinch/knockdown montage, ragdoll, despawn, heat — lives in the
 * AGTCCitizen adapter and is NOT covered here, exactly like FNpcContact.
 */
struct GTC_UE5_API FNpcVitals
{
    /** Health ceiling, floored at 0.0001 so Fraction() never divides by zero. */
    double MaxHealth = 100.0;

    /** Current health, always in [0, MaxHealth]. Seeded to MaxHealth on init. */
    double Health = 100.0;

    /** Construct with a toughness ceiling; floors MaxHealth and seeds Health full. */
    explicit FNpcVitals(double Maximum = 100.0);

    /** Down once health is spent. */
    bool IsDead() const { return Health <= 0.0; }

    /** Health fill fraction (Health / MaxHealth); safe because MaxHealth is floored. */
    double Fraction() const;

    /**
     * Apply a hit. Damage at/below zero, or a hit on an already-dead body, changes
     * nothing and returns Ignored. Otherwise health drops (clamped at zero) and the
     * result is Killed on the hit that empties it, else Wounded.
     */
    ENpcHitResult Apply(double Amount);

    /**
     * Normalised 0..1 measure of how hard a (non-lethal) round landed relative to
     * this body's toughness — Amount / MaxHealth, clamped. The adapter reads it to
     * pick a recoil flinch versus a flooring knockdown. Pure function of the args,
     * independent of current Health, so it is stable to test.
     */
    static double WoundSeverity(double Amount, double MaxHealth);
};
