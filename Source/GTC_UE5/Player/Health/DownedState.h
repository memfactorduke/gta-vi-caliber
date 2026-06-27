// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The downed / bleed-out grace window — the second chance a two-protagonist game
 * lives on. FPlayerHealthModel is binary: health hits 0 and you're dead. But the
 * co-op beat is "you DROP, you're bleeding out, and your partner has a few seconds
 * to haul you back up before you're gone". FDownedState is that window, a small FSM
 * that sits BETWEEN health and death: health owns hit points, this owns the grace
 * timer and who gets to close it (a revive, the bleed-out, or an enemy execution).
 *
 * Up -> Downed -> Dead. When health runs out the adapter calls GoDown, dropping the
 * player into Downed with a bleed-out clock; while Downed, Tick runs that clock and
 * a Revive (a partner reaching you) pulls them back to Up, while FinishOff (an enemy
 * executing them) or the clock expiring sends them to Dead. Second chances aren't
 * unlimited: each life allows MaxDowns drops, and going down once they're spent is
 * an instant kill — Reset (a respawn / safehouse) restores the budget.
 *
 * It's deliberately decoupled: the adapter wires "health == 0 -> GoDown", and on a
 * Revive restores some health and (since reviving a partner is the strongest bond
 * there is) nudges FPartnerBond — neither the health nor the bond core needs to know
 * this state exists.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same events over the same time always yield the same state — unit-tested
 * headless (Tests/DownedStateTest.cpp, prefix GTC.Player.Downed).
 *
 * PURE-CORE boundary: detecting health depletion, restoring HP / nudging the bond on
 * a revive, the ragdoll/crawl, the bleed-out vignette, and the execute prompt is the
 * DEFERRED adapter and is NOT covered by the tests. Units: BleedOutSeconds and Dt are
 * in seconds; BleedProgress is 0..1.
 */
struct FDownedState
{
    /** Where the player sits between full health and death. */
    enum class EState : uint8
    {
        Up,     // on their feet
        Downed, // bleeding out, revivable
        Dead,   // gone
    };

    /** Tuning for the grace window. */
    struct FParams
    {
        /** Seconds from going down to dead if nobody revives you. */
        double BleedOutSeconds = 20.0;
        /** Downs survivable per life; the (MaxDowns+1)-th drop is an instant kill. Min 0. */
        int32 MaxDowns = 2;
    };

    /** Install the tuning and reset to Up with a fresh downs budget. */
    void Configure(const FParams& InParams);

    /**
     * Health ran out — drop the player. From Up: enters Downed (bleed clock reset) and
     * spends a down, UNLESS the downs budget is already used up, in which case it's an
     * instant Dead. A no-op if already Downed or Dead.
     */
    void GoDown();

    /** A partner reached you in time: Downed -> Up (clock reset). A no-op otherwise. */
    void Revive();

    /** An enemy executed you while down: Downed -> Dead. A no-op otherwise. */
    void FinishOff();

    /**
     * Advance the bleed-out clock by `Dt` seconds (clamped >= 0). Only meaningful while
     * Downed; when the clock reaches BleedOutSeconds the player dies. A no-op when Up or
     * Dead.
     */
    void Tick(double Dt);

    /** Back to full standing with the downs budget restored (respawn / safehouse). */
    void Reset();

    EState State() const { return CurrentState; }
    bool IsUp() const { return CurrentState == EState::Up; }
    bool IsDowned() const { return CurrentState == EState::Downed; }
    bool IsDead() const { return CurrentState == EState::Dead; }

    /** Downs spent this life. */
    int32 DownsTaken() const { return Downs; }

    /** Fraction of the bleed-out elapsed, 0..1 (0 when not Downed). */
    double BleedProgress() const;

    /** Seconds left before bleed-out kills you (0 when not Downed). */
    double TimeLeft() const;

private:
    FParams Params;
    EState CurrentState = EState::Up;
    double BleedTimer = 0.0;
    int32 Downs = 0;
};
