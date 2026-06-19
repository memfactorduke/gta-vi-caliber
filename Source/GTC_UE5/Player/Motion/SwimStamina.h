// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure swim-stamina / oxygen meter for the player when in water.
 *
 * Direct port of the the reference `SwimStamina` (RefCounted) stateful instance at
 * `game/scripts/player/swim_stamina.gd`. No scene access — a plain value type
 * (same pattern as FGpsNavigation's stateless math, but this one holds the
 * oxygen/stamina state). The swim node owns one and feeds it
 * submersion/sprint/depth/time; this is the math layer ABOVE FSwimMotion:
 * oxygen drains while the head is under (faster the deeper you dive), refills at
 * the surface; stamina drains while swimming (more when sprinting) and recovers
 * while idle/floating.
 *
 * Double precision throughout, to match the the reference implementation float math. No engine
 * wiring; driving this from a real water volume is a DEFERRED Wave-3 adapter.
 *
 * NOTE: this system is marked UNUSED in the migration inventory, but it has a
 * live the reference test oracle (game/tests/unit/test_swim_stamina.gd), so it is ported
 * with full parity coverage per the Wave-1 "port any that have an oracle" rule.
 *
 * Parity coverage: Tests/SwimStaminaTest.cpp, prefix GTC.Player.Motion,
 * mirroring the oracle 1:1.
 */
struct GTC_UE5_API FSwimStamina
{
    /** Oxygen lost per second at the surface depth (depth 0), scaled up by pressure. */
    static constexpr double OxygenDrainPerSecond = 1.0;
    /** Oxygen regained per second of breathing at the surface (a gasp recovers fast). */
    static constexpr double OxygenRefillPerSecond = 5.0;
    /** Stamina lost per second of ordinary swimming. */
    static constexpr double StaminaDrainPerSecond = 4.0;
    /** Extra stamina lost per second while sprint-swimming, on top of the base drain. */
    static constexpr double SprintStaminaExtra = 8.0;
    /** Stamina regained per second while idle/floating (slower than it drains). */
    static constexpr double StaminaRecoverPerSecond = 3.0;
    /** Metres of depth that double the pressure (1 atm at surface, 2 atm here). */
    static constexpr double DepthPerAtmosphere = 10.0;
    /** Swim-speed multiplier once exhausted — you can still paddle, just slowly. */
    static constexpr double ExhaustedSpeedFactor = 0.5;
    /** Swim-speed multiplier while sprinting (only granted while stamina lasts). */
    static constexpr double SprintSpeedFactor = 1.6;

    /**
     * Construct a fresh meter — full breath and stamina. Both maxima are floored
     * at 0.0001 so the fraction getters never divide by zero, matching the Godot
     * `_init` defaults (20.0 oxygen, 100.0 stamina).
     */
    explicit FSwimStamina(double MaximumOxygen = 20.0, double MaximumStamina = 100.0);

    /**
     * Advance one frame. bIsUnderwater true means the head is under; Depth is how
     * far below the surface the head is, in metres (negative treated as 0). Oxygen
     * drains underwater (faster the deeper), refills at the surface. Stamina
     * drains while swimming — more when sprinting — and recovers while idle. Both
     * stay clamped to [0, max]. A negative Delta is treated as 0 (no-op).
     */
    void Update(bool bIsUnderwater, bool bIsSprinting, double Depth, double Delta);

    double Oxygen() const;
    double OxygenFraction() const;
    double Stamina() const;
    double StaminaFraction() const;

    /** Out of breath while still under: this is when drowning damage begins. */
    bool IsDrowning(bool bIsUnderwater) const;

    /** No stamina left — sprint is disabled and swim speed is throttled. */
    bool IsExhausted() const;

    /**
     * Drowning damage for this frame: Dps per second once oxygen is gone and the
     * head is under, zero while there is any breath left. Negative inputs guarded.
     */
    double DrownDamage(bool bIsUnderwater, double Delta, double Dps) const;

    /**
     * Effective swim speed: throttled to a crawl when exhausted, boosted while
     * sprinting — but the sprint boost only applies while stamina remains.
     */
    double SwimSpeed(double BaseSpeed, bool bIsSprinting) const;

    /**
     * Water pressure in atmospheres at Depth metres (1.0 at the surface, rising
     * linearly). The multiplier on oxygen drain. Negative depth clamps to surface.
     */
    double PressureAt(double Depth) const;

    /** Instant breath refill on reaching air (a surfacing gasp), stamina untouched. */
    void Surface();

    /** Back to a fresh dive: full breath and full stamina. */
    void Reset();

    double MaxOxygen = 20.0;
    double OxygenValue = 20.0;
    double MaxStamina = 100.0;
    double StaminaValue = 100.0;
};
