// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The per-protagonist SPECIAL ABILITY — the signature "fill a bar with skillful, violent
 * play, then unleash it" mechanic. It charges from kills, headshots, stunts, near-misses,
 * and sustained drifts, holds its charge until spent (no passive bleed), and while active
 * drains over a few seconds to grant a character-flavoured edge: Marksman slows the world
 * to a crawl for precision gunplay, Bruiser shrugs off damage and hits harder, Driver
 * sharpens the car's handling. It ties together combat, driving, and stunts — the systems
 * this loop has been building — into one expressive resource.
 *
 * Pairs with FCharacterRoster (each lead carries their own ability/kind), and the charge
 * events line up with the existing scorers (a confirmed kill from the weapon layer, a
 * banked FStuntScore trick, an FVehicleHandling drift, an FAimAssist/near-miss).
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG. The
 * player adapter calls AddCharge(...) on those gameplay events, Activate() on the input,
 * Update(dt) each frame, and multiplies the returned effect factors into time dilation /
 * incoming + outgoing damage / vehicle handling. Times are seconds; charge is a 0..1
 * fraction of the bar. Unit-tested headless (Systems/Ability/Tests/SpecialAbilityTest.cpp,
 * prefix GTC.Systems.Ability).
 */
enum class EAbilityKind : uint8
{
    Marksman, // bullet-time: the world slows, the player's aim does not
    Bruiser,  // tank: take less, hit harder
    Driver,   // wheelman: grip, speed, and a touch of focus time
};

struct GTC_UE5_API FSpecialAbility
{
    /** Full bar drains in 1/DrainPerSec seconds of use (0.2 => ~5s). */
    static constexpr double DrainPerSec = 0.2;
    /** Minimum charge needed to trigger the ability. */
    static constexpr double MinActivateCharge = 0.1;

    // Charge gained per gameplay event, as a fraction of the bar.
    static constexpr double ChargeForKill = 0.10;
    static constexpr double ChargeForHeadshot = 0.18;
    static constexpr double ChargeForStunt = 0.12;
    static constexpr double ChargeForNearMiss = 0.05;
    static constexpr double ChargePerDriftSec = 0.04;

    EAbilityKind Kind = EAbilityKind::Marksman;
    double Charge = 0.0; // 0..1
    bool bActive = false;

    /** Add charge (a 0..1 fraction); negatives ignored, result clamped to a full bar. */
    void AddCharge(double Amount);

    /** Enough in the tank to trigger it (and not already running). */
    bool CanActivate() const { return !bActive && Charge >= MinActivateCharge; }

    /** Trigger the ability; returns false (no-op) if it can't be activated. */
    bool Activate();

    /** End it early (input released). Leaves the remaining charge intact. */
    void Deactivate() { bActive = false; }

    /** Advance one frame: while active, drain the bar and auto-end when it hits empty. */
    void Update(double Dt);

    bool IsActive() const { return bActive; }
    double Fraction() const { return Charge; }

    // --- Effect factors (all return the neutral 1.0 while inactive) ---

    /** World time scale while active (Marksman slows it). Multiply into the global dilation. */
    double TimeDilation() const;
    /** Multiplier on damage the player TAKES (Bruiser soaks it). */
    double DamageTakenMultiplier() const;
    /** Multiplier on damage the player DEALS (Bruiser hits harder). */
    double DamageDealtMultiplier() const;
    /** Multiplier on vehicle handling grip/top-speed (Driver). */
    double HandlingBoost() const;
};
