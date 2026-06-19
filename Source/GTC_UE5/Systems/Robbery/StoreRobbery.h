// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The store stick-up — walk into a liquor store or gas station, level a gun at the clerk,
 * and watch them empty the register while you race their hand to the silent alarm. A
 * signature open-world crime that nothing modelled yet ("mugging" was only an ambient-event
 * id with no mechanic behind it).
 *
 * The loop it captures: keeping a weapon trained on the clerk builds their FEAR; once fear
 * crosses the compliance line they COOPERATE and the register drains; the moment you look
 * away or lower the gun, fear ebbs and — if they're back to defiant — a silent-alarm
 * countdown starts that, if it runs out, raises your wanted level. So the tension is hold
 * the threat long enough to clear the till, but the longer you take the more chances the
 * clerk gets to be brave.
 *
 * PURE-CORE: scene-free, deterministic, double precision, no engine coupling, no RNG. The
 * robbery adapter (a trigger volume on the counter) constructs one with the till total,
 * each frame reads whether the player is threatening the clerk + how intimidating they are
 * (FStoreRobbery::IntimidationFrom) and calls Update(dt, ...), pays out CashTaken to
 * PlayerStats, and on bAlarmRaised raises FWantedSystem heat. Times are seconds; cash is
 * the adapter's own currency unit. Unit-tested headless
 * (Systems/Robbery/Tests/StoreRobberyTest.cpp, prefix GTC.Systems.Robbery).
 */
struct GTC_UE5_API FStoreRobbery
{
    /** Fear at/above which the clerk stops resisting and starts emptying the till. */
    static constexpr double ComplianceThreshold = 0.6;
    /** Fear gained per second of being threatened, scaled by intimidation strength. */
    static constexpr double FearGainPerSec = 0.8;
    /** Fear lost per second when the threat lets up. */
    static constexpr double FearDecayPerSec = 0.4;
    /** Seconds a fully-cooperating clerk takes to empty the whole register. */
    static constexpr double EmptyDurationSec = 3.0;
    /** Seconds a defiant clerk needs, uninterrupted, to reach the silent alarm. */
    static constexpr double AlarmCountdownSec = 4.0;
    /** Wanted heat a tripped silent alarm generates (feed FWantedSystem). */
    static constexpr double AlarmHeat = 3.0;

    /**
     * How intimidating the player is to the clerk, 0..1: unarmed menace is feeble; an armed
     * robber is scarier; a gun actually LEVELLED at the clerk is scariest, and a higher
     * weapon tier (0..1) adds to it.
     */
    static double IntimidationFrom(bool bArmed, double GunTier01, bool bAimedAtClerk);

    // --- Live robbery state ---
    double RegisterTotal = 0.0;       // cash in the till at the start
    double CashTaken = 0.0;           // cash grabbed so far
    double Fear = 0.0;                // clerk's fear, 0..1
    double AlarmCountdown = AlarmCountdownSec; // time left before a defiant clerk alarms
    bool bAlarmRaised = false;        // silent alarm tripped (raise wanted)

    /**
     * Advance one frame. While bThreatening, fear climbs by Intimidation; otherwise it ebbs.
     * A cooperating clerk (fear >= ComplianceThreshold) drains the register and can't reach
     * the alarm; a defiant one runs the alarm countdown down, tripping it at zero.
     */
    void Update(double Dt, bool bThreatening, double Intimidation);

    bool IsCooperating() const { return Fear >= ComplianceThreshold; }
    bool IsEmptied() const { return RegisterTotal > 0.0 && CashTaken >= RegisterTotal - 1e-9; }
    double Remaining() const { return RegisterTotal > CashTaken ? RegisterTotal - CashTaken : 0.0; }
    double Progress() const;
};
