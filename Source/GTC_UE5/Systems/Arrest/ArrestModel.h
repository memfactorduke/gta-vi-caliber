// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure "Busted" resolution — the UE 5.7 port of Godot's ArrestModel (RefCounted, PURE).
 * The arrest counterpart to a Wasted (death): when the police corner the player (hold
 * them within catch range while wanted) for a short grapple window, the bust lands —
 * the player respawns stripped of a slice of cash and all heat.
 *
 * Static, scene-free, RNG-free — exactly like the Godot static helpers. Plain C++ value
 * type (no UObject); the UArrestSubsystem (the arrest_controller port) owns the grapple
 * timer and drives these helpers from live positions. Headless-testable: parity oracle
 * game/tests/unit/test_arrest_model.gd (15 assertions, mapped 1:1).
 *
 * Parity note: all math is `double`. Constants mirror the Godot const literals exactly.
 * cash_after_bust uses floor (Godot floori) of float(wallet) * (1 - clamp(penalty)),
 * floored at 0; bust_fee is the complement (wallet - kept), floored at 0.
 */
struct GTC_UE5_API FArrestModel
{
    /** Seconds the player must be cornered before the cuffs go on (Godot DEFAULT_GRAPPLE_TIME). */
    static constexpr double DefaultGrappleTime = 1.5;

    /** Fraction of the wallet forfeited on a bust (Godot DEFAULT_CASH_PENALTY). */
    static constexpr double DefaultCashPenalty = 0.10;

    /**
     * Is an officer cornering the player this tick — wanted (stars > 0) AND within catch
     * range (distance <= catch_distance). Mirrors Godot ArrestModel.cornered.
     */
    static bool Cornered(int32 Stars, double Distance, double CatchDistance);

    /**
     * Advance the grapple timer: counts up while cornered, bleeds back off (never below
     * zero) the instant the player breaks free. Mirrors Godot ArrestModel.tick_grapple:
     * cornered ? max(t+dt, 0) : max(t-dt, 0).
     */
    static double TickGrapple(double TimeHeld, bool bIsCornered, double Dt);

    /**
     * A bust lands once the player has been cornered continuously for grapple_time:
     * grapple_time > 0 AND time_held >= grapple_time. Mirrors Godot ArrestModel.is_busted.
     */
    static bool IsBusted(double TimeHeld, double GrappleTime);

    /**
     * Cash kept after a bust: floor(wallet * (1 - clamp(penalty, 0, 1))), floored at 0.
     * Mirrors Godot ArrestModel.cash_after_bust.
     */
    static int32 CashAfterBust(int32 Wallet, double PenaltyFraction);

    /**
     * The cash a bust takes (wallet minus what's kept), never negative.
     * Mirrors Godot ArrestModel.bust_fee.
     */
    static int32 BustFee(int32 Wallet, double PenaltyFraction);
};
