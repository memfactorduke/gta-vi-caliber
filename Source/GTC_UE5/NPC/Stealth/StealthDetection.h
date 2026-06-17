// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure stealth-detection awareness meter — the "eye" that fills up before an NPC
 * or cop actually spots the player. Sits one layer above the instantaneous
 * can-I-see-it FOV check: this is the TIME-BASED meter built on top, so a target
 * glimpsed for a single frame doesn't flip straight to ALERTED.
 *
 * Direct port of the Godot `StealthDetection` (RefCounted) at
 * `game/scripts/npc/stealth_detection.gd`. A stateful instance, one per watcher.
 * Awareness is 0..1 and walks UNAWARE -> SUSPICIOUS -> ALERTED as the watcher
 * keeps eyes on the player, and decays back down when sight is lost. ALERTED is
 * sticky once reached: a full meter means "spotted", and a momentary flicker of
 * sight doesn't un-spot the player — only Reset() clears it.
 *
 * Double precision throughout, to match the GDScript float math. Defensive:
 * negative delta and out-of-range inputs are clamped, not trusted. Unit-tested
 * headless via the parity oracle (Tests/StealthDetectionTest.cpp, prefix
 * GTC.NPC.Stealth).
 *
 * PURE-CORE boundary: this is the pure awareness/detection-meter MATH only —
 * fill/decay rates, the visibility scaling of fill, the SUSPICIOUS threshold,
 * the sticky ALERTED latch, and the pure DetectionSpeed() visibility helper
 * (distance falloff, crouch cut, movement gain) over caller-supplied inputs.
 * The AI Perception "Sight" integration — the actual LOS/visibility query that
 * decides bCanSeePlayer and computes a real Visibility — is the DEFERRED Wave-3
 * adapter: NOT implemented here and NOT covered by the parity tests. The model
 * takes bCanSeePlayer / Visibility / Delta as plain inputs; the adapter would
 * feed those from the perception system and route the resulting State into AI.
 */

/** Discrete watcher awareness state. Godot ordinal order preserved. */
enum class EStealthState : uint8
{
    Unaware,
    Suspicious,
    Alerted,
};

struct GTC_UE5_API FStealthDetection
{
    /**
     * `FillRate` is awareness/second gained at full visibility while seen;
     * `DecayRate` is awareness/second lost while not seen. `SuspiciousThreshold`
     * (0..1) is where UNAWARE becomes SUSPICIOUS. Rates are floored at zero and
     * the threshold clamped into (0,1) so a degenerate config can't misbehave.
     */
    explicit FStealthDetection(double FillRate, double DecayRate, double SuspiciousThreshold = 0.4);

    /**
     * Advance one frame. `bCanSeePlayer` gates everything — when false the meter
     * decays. `Visibility` (0..1) scales the fill speed: a close, lit, moving
     * target fills fast; a distant, dark, crouched one barely at all (Visibility
     * 0 can't fill even while bCanSeePlayer is true — you see a shape but can't
     * make it out). Negative delta is ignored so time never runs backwards. Once
     * ALERTED the meter stays pinned and sticky; only Reset() releases it.
     */
    void Update(bool bCanSeePlayer, double Visibility, double Delta);

    /** Current awareness, 0..1. */
    double Awareness() const;

    /** Current discrete state (see EStealthState). */
    EStealthState State() const;

    bool IsAlerted() const;

    bool IsSuspicious() const;

    /**
     * Pure visibility helper (0..1) the caller can feed into Update(). Visibility
     * falls off linearly with distance/`MaxRange` (out of range -> 0), is cut
     * while the target is crouched, and raised while it is moving. The result is
     * clamped 0..1 so any mix of factors stays a valid visibility.
     */
    static double DetectionSpeed(double Distance, double MaxRange, bool bTargetCrouched, bool bTargetMoving);

    /** Wipe the meter back to UNAWARE and clear the sticky ALERTED latch. */
    void Reset();

private:
    double FillRate = 0.0;
    double DecayRate = 0.0;
    double SuspiciousThreshold = 0.4;
    double AwarenessValue = 0.0;
    bool bAlerted = false;
};
