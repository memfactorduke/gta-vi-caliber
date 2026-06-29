// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Grabbing a hostage as a human shield — the heist-gone-wrong standoff. When the
 * cops have you cornered, snatching a bystander buys you a moment: while you hold a
 * shield they hold fire. But it's a race you can only delay, never win — the hostage
 * struggles to break loose, faster the more you drag them around, and your only
 * lever is to intimidate them back into line. FHostageStandoff is that micro-loop:
 * a struggle that fills toward break-free and an intimidation that pushes it down
 * with diminishing returns.
 *
 * The skill is stretching the shield long enough to back to cover or a getaway car.
 * Struggle rises every tick (faster while moving); Intimidate shoves it down, but
 * each shout buys less than the last (geometric falloff), so you can't hold a
 * hostage forever. When struggle tops out the hostage breaks free and the shield's
 * gone. The "police hold fire" effect is just the adapter reading IsShieldActive(),
 * so this core stays a self-contained timer and never reaches into the police AI.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same grabs/intimidations over the same time always yield the same standoff —
 * unit-tested headless (Tests/HostageStandoffTest.cpp, prefix GTC.Systems.Hostage).
 *
 * PURE-CORE boundary: grabbing the actual bystander pawn, suppressing police fire
 * while IsShieldActive(), the intimidate input/animation, and freeing the hostage is
 * the DEFERRED adapter and is NOT covered by the tests. Units: Struggle is 0..1;
 * rates are per-second; Dt is seconds.
 */
struct FHostageStandoff
{
    /** Standoff state. */
    enum class EState : uint8
    {
        None,      // no hostage in hand
        Holding,   // shield up
        BrokeFree, // the hostage wrenched loose
        Released,  // you let them go
    };

    /** Tuning for how hard a hostage is to hold. */
    struct FParams
    {
        /** Struggle gained per second while held and standing still. */
        double StrugglePerSec = 0.06;
        /** Multiplier on the struggle rate while you're dragging the hostage around. */
        double MoveStruggleMult = 2.0;
        /** Struggle the FIRST intimidation pushes back down. */
        double IntimidateRelief = 0.4;
        /** Each subsequent intimidation is this fraction as effective as the last (geometric falloff). */
        double IntimidateFalloff = 0.6;
    };

    /** Install the tuning. The standoff starts with no hostage. */
    void Configure(const FParams& InParams);

    /** Grab a (fresh) hostage: shield up, struggle reset, intimidation history cleared. */
    void Grab();

    /**
     * Advance one tick. While Holding, struggle rises (faster if `bMoving`); when it
     * tops out the hostage breaks free. `Dt` clamped >= 0; a no-op when not Holding or
     * for a non-positive Dt.
     */
    void Update(bool bMoving, double Dt);

    /**
     * Shout the hostage down: pushes struggle back by IntimidateRelief scaled by the
     * geometric falloff for how many times you've already done it, and counts the
     * intimidation. Returns the relief actually applied (0 when not Holding).
     */
    double Intimidate();

    /** Let the hostage go (Holding -> Released). A no-op otherwise. */
    void Release();

    EState State() const { return CurrentState; }
    bool IsHolding() const { return CurrentState == EState::Holding; }
    /** True while the hostage is still a working shield (the adapter suppresses police fire on this). */
    bool IsShieldActive() const { return CurrentState == EState::Holding; }

    /** How close the hostage is to breaking loose, 0..1 (0 when not Holding). */
    double StruggleFraction() const;

    /** How many times you've intimidated the current hostage. */
    int32 Intimidations() const { return IntimidateCount; }

private:
    FParams Params;
    EState CurrentState = EState::None;
    double Struggle = 0.0;
    int32 IntimidateCount = 0;
};
