// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * FPlayerHealthModel — pure player health with regeneration.
 *
 * Ported 1:1 from Godot `PlayerHealthModel` (a RefCounted, PURE) in
 * `game/scripts/systems/player_health_model.gd`, with its parity oracle
 * `game/tests/unit/test_player_health_model.gd` (18 funcs). open-world-style: health
 * regenerates back to full a few seconds after the last hit; body armor soaks
 * incoming damage 1:1 before health.
 *
 * No scene / UObject access — headless-testable. The owning component
 * (UPlayerHealthComponent) holds one and feeds it damage and per-frame `Tick`
 * time, exactly as the Godot `PlayerHealth` Node fed the model.
 *
 * Parity notes vs Godot:
 *  - `double` throughout (Godot `float` == 64-bit). No Z-up remap (no vectors).
 *  - `_init(maximum, regen, delay, armor_max)` defaults preserved: 100/10/5/100.
 *  - `max_health = maxf(maximum, 0.0001)` floor preserved so Fraction() never
 *    divides by zero.
 *  - Regen-slice rule preserved verbatim: on the frame that crosses regen_delay,
 *    only the slice of `delta` that fell PAST the delay regenerates, so the
 *    crossing frame doesn't grant a full frame of regen for cooldown time.
 *  - `heal`/`tick` are no-ops while dead (no silent revive); `revive` restores
 *    full health, clears armor, and sets the regen timer to regen_delay.
 */
struct GTC_UE5_API FPlayerHealthModel
{
    // --- Tunables (Godot _init args) -------------------------------------------

    /** Health ceiling, floored at 0.0001 (Godot maxf(maximum, 0.0001)). */
    double MaxHealth = 100.0;

    /** HP regenerated per second once the regen delay has elapsed. */
    double RegenRate = 10.0;

    /** Seconds after the last hit before regeneration begins. */
    double RegenDelay = 5.0;

    /** Body-armor ceiling, floored at 0 (Godot maxf(armor_max, 0.0)). */
    double MaxArmor = 100.0;

    // --- Live state ------------------------------------------------------------

    /** Current health, always in [0, MaxHealth]. Seeded to MaxHealth on init. */
    double Health = 100.0;

    /** Current body-armor pool, always in [0, MaxArmor]. Starts at 0. */
    double Armor = 0.0;

    /**
     * Construct mirroring Godot `_init(maximum, regen, delay, armor_max)`:
     * floors MaxHealth at 0.0001 and MaxArmor at 0, seeds Health to MaxHealth.
     */
    explicit FPlayerHealthModel(
        double Maximum = 100.0, double Regen = 10.0, double Delay = 5.0, double ArmorMax = 100.0);

    bool IsDead() const { return Health <= 0.0; }

    /**
     * Apply damage (negatives ignored) and reset the regen timer. Armor soaks
     * damage first, 1:1, the remainder hits health. A hit on an already-dead
     * model is a no-op. Returns true only on the hit that drops health to zero.
     */
    bool Apply(double Amount);

    /** Add body armor from a pickup (negatives ignored), clamped to MaxArmor. */
    void AddArmor(double Amount);

    /** Armor fill fraction in [0,1]; 0 when MaxArmor <= 0. */
    double ArmorFraction() const;

    /**
     * Advance one frame: regenerate once RegenDelay has elapsed since the last
     * hit. No-op while dead. Only the slice of `Delta` past the delay regens.
     */
    void Tick(double Delta);

    /**
     * Restore health (negatives ignored), capped at MaxHealth. Does NOT
     * resurrect: a heal on a dead model is a no-op.
     */
    void Heal(double Amount);

    /** Health fill fraction (Health / MaxHealth). */
    double Fraction() const;

    /** Full revive: health to MaxHealth, armor cleared, regen timer = RegenDelay. */
    void Revive();

    // --- Test seam -------------------------------------------------------------

    /** Seconds since the last hit. Drives the regen gate; exposed for parity tests. */
    double GetSinceDamage() const { return SinceDamage; }

private:
    /** Godot `_since_damage`: time accumulated since the last Apply(). */
    double SinceDamage = 0.0;
};
