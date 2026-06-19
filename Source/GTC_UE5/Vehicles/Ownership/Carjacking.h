// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure model for the iconic carjack: yanking a driver out of a car. Two halves — static
 * geometry helpers (can the player reach the door? which side? is it a crime?) and a
 * small STATEFUL struggle timer (the seconds spent wrestling a driver out before the
 * player can climb in).
 *
 * No scene access — a node owns one, feeds it Tick(Delta) each frame, and reads
 * Progress()/IsComplete(). All math is in the XZ plane (Y is up, mirroring the Godot
 * source) and defensive: zero-length and negative inputs never produce NaN.
 *
 * No RNG; fully deterministic — the struggle is a plain timer scaled by a driver
 * toughness multiplier, never a random roll.
 *
 * Crime distinction: jacking an OCCUPIED car is a crime (you assault the driver in
 * public, so it draws heat); hopping into an EMPTY parked car is theft-lite and draws
 * zero heat here (the wanted system scores the theft elsewhere).
 */
class GTC_UE5_API Carjacking
{
public:
    /** Default heat a witnessed occupied carjack feeds the wanted system. */
    static constexpr double DefaultJackHeat = 2.0;

    /** Geometric epsilon for zero-length / dead-centre checks. */
    static constexpr double Eps = 1.0e-4;

    // --- Static geometry / crime helpers ----------------------------------

    /** Drop the vertical (Y) component — door geometry is solved on the ground plane. */
    static FVector Ground(const FVector& V);

    /** True when the player is close enough (on the flat plane) to grab a door handle. */
    static bool CanReach(const FVector& PlayerPos, const FVector& CarPos, double ReachRadius);

    /**
     * Which side the player approaches from, via lateral projection onto the car's right
     * vector: -1 = driver side (left), +1 = passenger side (right), 0 = dead centre /
     * undefined facing. CarForward need not be normalised.
     */
    static int32 DoorSide(const FVector& CarPos, const FVector& CarForward, const FVector& PlayerPos);

    /** Is this jack a crime that draws heat? Occupied = yes, empty parked car = no. */
    static bool IsOccupiedCrime(bool bCarHasDriver);

    /** Heat for a jack: BaseHeat only when occupied (floored at 0); 0 for an empty car. */
    static double HeatForJack(bool bCarHasDriver, double BaseHeat = DefaultJackHeat);

    /**
     * How much a tougher driver lengthens the struggle. DriverToughness is a 0..1
     * resistance rating; returns a multiplier in 1.0..2.0 applied to the base duration
     * BEFORE construction (e.g. Carjacking(Base * ResistModifier(t))).
     */
    static double ResistModifier(double DriverToughness);

    // --- Struggle timer ---------------------------------------------------

    /**
     * Duration = seconds to wrestle the driver out (scale it up front with
     * ResistModifier). Non-positive durations are floored to a tiny positive value so
     * progress can't divide by zero and completes at once.
     */
    explicit Carjacking(double Duration = 1.2);

    /** Start (or restart) the struggle from zero. Idempotent re-arm: clears prior completion. */
    void Begin();

    /**
     * Advance the wrestle by Delta seconds. No-op before Begin(), once complete, or for a
     * negative Delta. Flips to complete once accumulated time reaches the duration.
     */
    void Tick(double Delta);

    /** Struggle fraction, 0.0 at the grab and clamped to 1.0 once the driver is out. */
    double Progress() const;

    /** True once the driver has been ejected and the player may climb in. */
    bool IsComplete() const;

    /** Player walked away mid-jack: abort. Stays incomplete; further ticks do nothing. */
    void Cancel();

    /** Full reset to the idle, never-begun state. */
    void Reset();

    /** Current struggle duration in seconds (floored positive). */
    double Duration() const { return JackDuration; }

private:
    double JackDuration = 1.2;
    double Elapsed = 0.0;
    bool bActive = false;
    bool bComplete = false;
};
