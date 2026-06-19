// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure health / damage-state machine for a vehicle: the timeline where a beaten-up car
 * smokes, then catches fire, then explodes.
 *
 * STATEFUL instance, no scene access — a Car node owns one and feeds it damage and time,
 * so the burn-down/explosion curve is unit-tested (Tests/VehicleHealthTest.cpp). This is
 * the HEALTH state machine; the crash-impact math lives elsewhere and is unrelated.
 *
 * State boundaries, as a fraction of max health:
 *   PRISTINE   >= 0.66
 *   DAMAGED    [0.33, 0.66)
 *   SMOKING    [fire_threshold, 0.33)   (skipped if fire_threshold >= 0.33)
 *   ON_FIRE    (0, fire_threshold)      — starts the explosion fuse
 *   WRECKED    health == 0, or the fuse has elapsed (irreversible until reset)
 *
 * Parity notes:
 * - double throughout; identical literals/tolerances to the Godot oracle.
 * - Godot's INF (time_to_explosion before fire) maps to std::numeric_limits<double>::infinity().
 *
 * Deferred Wave-3 adapters (NOT implemented/tested here): the vehicle-state component that
 * owns this model, the Chaos/cosmetic smoke+fire VFX, and the explosion damage application.
 */
class GTC_UE5_API FVehicleHealth
{
public:
    /** Health state ladder. Scoped enum keeps these names out of the global namespace. */
    enum class EState : uint8
    {
        Pristine,
        Damaged,
        Smoking,
        OnFire,
        Wrecked
    };

    /** Boundary above which the car is PRISTINE (fraction of max health). */
    static constexpr double DamagedFraction = 0.66;
    /** Boundary below which the car is SMOKING (fraction of max health). */
    static constexpr double SmokingFraction = 0.33;
    /** Seconds the car burns once ON_FIRE before it explodes into WRECKED. */
    static constexpr double DefaultFuse = 5.0;
    /** Health chipped off per second while burning (cosmetic; the fuse triggers the explosion). */
    static constexpr double BurnRate = 0.0;

    double MaxHealthValue = 0.0;
    double FireThresholdFraction = 0.0;
    double FuseDuration = 0.0;

    /**
     * Construct with a starting max health, fire threshold (capped to [0, SmokingFraction] so a
     * merely-DAMAGED car can't be routed straight to ON_FIRE), and explosion fuse seconds.
     */
    explicit FVehicleHealth(
        double StartingMaxHealth = 1000.0,
        double FireThreshold = 0.2,
        double FuseSeconds = DefaultFuse);

    /** Apply damage (negative ignored). Health floors at 0; crossing into ON_FIRE arms the fuse. */
    void ApplyDamage(double Amount);

    /** Advance time (negative delta ignored). Only meaningful while ON_FIRE: burns the fuse down. */
    void Tick(double Delta);

    double Health() const;

    double HealthFraction() const;

    EState State() const;

    bool IsOnFire() const;

    bool IsWrecked() const;

    /** Seconds of fuse left before the explosion. +INF when not yet ON_FIRE; 0.0 once WRECKED. */
    double TimeToExplosion() const;

    /** One-shot: true exactly once after the explosion, then self-clears. */
    bool JustExploded();

    /** Restore to full health and PRISTINE; disarms the fuse. */
    void Repair();

    void Reset();

private:
    double HealthValue = 0.0;
    EState StateValue = EState::Pristine;
    double FuseRemaining = 0.0;
    bool bJustExploded = false;

    void Explode();
    void RefreshState();
    /** Move into ON_FIRE, arming the fuse only on the transition. */
    void EnterFire();
};
