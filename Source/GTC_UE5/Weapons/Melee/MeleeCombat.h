// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Hand-to-hand and melee-weapon brawling — the fist-fight layer that makes a street
 * scrap feel like a fight and not a damage-number exchange. Light jabs you can chain into
 * a rising combo, heavy swings that cost more, a block that soaks most of a blow, a
 * tight perfect-parry window that turns a defended hit into a punishing counter, and a
 * stamina pool that punishes mashing — gas out and your punches go limp.
 *
 * The ranged side of combat already has FWeaponBallistics + FAimAssist + the FCombatAi
 * decision brain; this is the missing melee counterpart, used both for the player's
 * fists/bats and for an NPC brawler.
 *
 * Two halves:
 *   - Static fight math (no state): per-strike damage with combo + exhaustion scaling,
 *     block reduction, perfect-parry detection, counter damage.
 *   - A stateful FMeleeFighter: the stamina pool + live combo counter. The adapter calls
 *     Strike() on an input, Tick() each frame (stamina regen + combo-window decay), and
 *     OnHitTaken() when the fighter eats a blow (which breaks the combo).
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG.
 * The MeleeController adapter resolves reach/hit detection and plays montages; it feeds
 * these helpers strike types + timings and applies the returned damage. Times are
 * seconds. Unit-tested headless (Weapons/Melee/Tests/MeleeCombatTest.cpp, prefix
 * GTC.Weapons.Melee).
 */
enum class EMeleeStrike : uint8
{
    Light, // fast jab — cheap, chains easily, low damage
    Heavy, // committed swing — pricey, big damage, breaks a block harder
};

struct GTC_UE5_API FMeleeCombat
{
    // --- Tunables ---
    static constexpr double LightDamage = 8.0;
    static constexpr double HeavyDamage = 20.0;
    static constexpr double LightStaminaCost = 8.0;
    static constexpr double HeavyStaminaCost = 22.0;

    /** Each chained hit adds this to the combo multiplier... */
    static constexpr double ComboStep = 0.15;
    /** ...up to this ceiling (so a long combo tops out rather than runs away). */
    static constexpr double MaxComboMultiplier = 2.5;

    /** Fraction of a blocked blow that still gets through (a normal, non-perfect block). */
    static constexpr double BlockRetention = 0.25;
    /** A heavy strike bullies through a block harder than a light one does. */
    static constexpr double HeavyBlockRetention = 0.45;

    /** Block raised within this window (s) before the blow lands = a perfect parry. */
    static constexpr double ParryWindow = 0.2;
    /** A perfect parry reflects this fraction of the incoming damage back as a counter. */
    static constexpr double CounterScale = 1.5;

    /** Below this stamina fraction the fighter is gassed; damage scales down to this floor. */
    static constexpr double ExhaustionFloor = 0.5;

    // --- Static fight math ---

    /** Base damage of a strike before combo / exhaustion scaling. */
    static double BaseDamage(EMeleeStrike Strike);
    /** Stamina a strike costs. */
    static double StaminaCost(EMeleeStrike Strike);

    /** Combo multiplier for `PriorHits` already landed in the current chain (0 -> 1.0). */
    static double ComboMultiplier(int32 PriorHits);

    /** How much damage tired arms still deliver: 1.0 at full stamina easing to
     *  ExhaustionFloor at empty. */
    static double ExhaustionFactor(double StaminaFraction);

    /** Full damage of a strike: BaseDamage x ComboMultiplier(PriorHits) x
     *  ExhaustionFactor(StaminaFraction). */
    static double StrikeDamage(EMeleeStrike Strike, int32 PriorHits, double StaminaFraction);

    /** True if a block raised `BlockHeldSeconds` before the blow lands counts as a perfect
     *  parry (raised no earlier than ParryWindow ago — last-instant timing). */
    static bool IsPerfectParry(double BlockHeldSeconds);

    /**
     * Damage that actually lands on a defender. A perfect parry negates it entirely
     * (0); a normal block lets through the (heavier-for-heavy) retained fraction; no
     * block lets it all through.
     */
    static double IncomingAfterBlock(double Incoming, EMeleeStrike Strike, bool bBlocking, bool bPerfectParry);

    /** Counter damage dealt back to the attacker on a perfect parry (0 if not perfect). */
    static double CounterDamage(double Incoming, bool bPerfectParry);

    /**
     * The brawler's live state: a stamina pool and the running combo. Stateful; the
     * adapter owns one per fighter.
     */
    struct GTC_UE5_API FMeleeFighter
    {
        double MaxStamina = 100.0;
        double Stamina = 100.0;
        double StaminaRegenPerSec = 15.0;
        /** Seconds a combo survives between hits before it lapses. */
        double ComboWindow = 1.2;

        int32 ComboCount = 0;   // hits landed in the current chain
        double ComboTimer = 0.0; // seconds left before the chain lapses

        double StaminaFraction() const { return MaxStamina > 0.0 ? Stamina / MaxStamina : 0.0; }
        bool IsExhausted() const { return StaminaFraction() < FMeleeCombat::ExhaustionFloor; }
        bool CanStrike(EMeleeStrike Strike) const { return Stamina >= FMeleeCombat::StaminaCost(Strike); }

        /**
         * Throw a strike: returns the damage dealt (0 if too gassed to throw it). Spends
         * stamina, extends the combo (using the hits already landed for the multiplier),
         * and refreshes the combo window.
         */
        double Strike(EMeleeStrike Strike);

        /** Advance one frame: regenerate stamina (capped) and decay the combo window;
         *  the chain lapses to 0 when the window runs out. */
        void Tick(double Dt);

        /** The fighter ate a blow — the combo breaks. */
        void OnHitTaken();
    };
};
