// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * The player's social-media following — the clout economy the setting runs on.
 * The world is thick with influencer satire (the citizens livestream, the radio
 * sells follows) but the player has no follower count of their own to grow.
 * FCloutLedger is that number and the rules that move it: post content (a stunt
 * clip, a landmark selfie, a caught-on-camera crime), and your following rises or
 * falls by how good the post is, how big you already are, and how often you post.
 * It's a side progression that influencer gigs / brand deals can later gate on.
 *
 * "Going viral" has to FEEL random but a pure-core has to BE deterministic, so
 * virality is a function of the inputs, not a dice roll:
 *   - REACH grows with the audience you already have (the network effect): the same
 *     post lands harder once you're big.
 *   - APPEAL (0..1, the post's quality) scales the gain above a flop line; a post
 *     below that line is a FLOP that actually costs you followers.
 *   - crossing the VIRAL appeal threshold multiplies the gain — the blow-up.
 *   - FATIGUE damps gain when you spam: posting again before the audience has
 *     rested earns a fraction of what a well-spaced post would.
 * And while you're offline the audience cools — Advance bleeds a little of the
 * following per idle hour, so clout is kept, not banked forever.
 *
 * Follower counts cross TIER thresholds (Unknown -> Nano -> Micro -> Macro ->
 * Celebrity) the gig/brand-deal layer reads to unlock opportunities.
 *
 * GTC-original pure-core: scene-free, deterministic, double precision, no UObject.
 * The same posts in the same order always yield the same following — unit-tested
 * headless (Tests/CloutLedgerTest.cpp, prefix GTC.Systems.Clout).
 *
 * PURE-CORE boundary: deciding what counts as a "post" and scoring its Appeal from
 * the act (stunt air time, landmark, kill-cam), showing the follower HUD, and
 * gating gigs on Tier is the DEFERRED adapter and is NOT covered by the tests.
 * Units: followers are a count; Advance Dt and the fatigue window are in hours;
 * Appeal is dimensionless 0..1.
 */
struct GTC_UE5_API FCloutLedger
{
    /** Audience size brackets the gig/brand-deal layer gates on. */
    enum class ETier : uint8
    {
        Unknown,   // nobody knows you
        Nano,      // a few hundred
        Micro,     // thousands
        Macro,     // tens/hundreds of thousands
        Celebrity, // the big leagues
    };

    /** Tuning for how clout grows and cools. Defaults give a plausible early-game curve. */
    struct FParams
    {
        /** Followers a perfect, well-rested post lands with zero existing audience. */
        double BaseReach = 200.0;
        /** How much each existing follower adds to a post's reach (the network effect). */
        double AudienceFactor = 0.05;
        /** Appeal at/under which a post is a FLOP and loses followers. */
        double FlopAppeal = 0.25;
        /** Fraction of reach lost by the worst possible flop (Appeal 0). */
        double FlopLoss = 0.5;
        /** Appeal at/above which a post can go viral. */
        double ViralAppeal = 0.8;
        /** Gain multiplier at Appeal 1 (interpolated up from 1x at ViralAppeal). */
        double ViralMultiplier = 5.0;
        /** Hours of rest after which a post earns full reach (sooner is fatigued). */
        double FatigueWindow = 3.0;
        /** Floor on the fatigue factor — a back-to-back post still earns this fraction. */
        double FatigueFloor = 0.15;
        /** Fraction of the following the audience sheds per idle hour. */
        double DecayPerHour = 0.01;

        /** Tier floors (followers). */
        double NanoAt = 1000.0;
        double MicroAt = 25000.0;
        double MacroAt = 250000.0;
        double CelebrityAt = 1000000.0;
    };

    /** Install the tuning and reset to zero followers, rested. */
    void Configure(const FParams& InParams);

    /**
     * Publish a post of quality `Appeal` (clamped to [0,1]) and return the NET change
     * in followers (positive for a hit, negative for a flop). Mutates the following and
     * marks the audience freshly posted-to (resetting fatigue). Gain = reach (grown by
     * your audience) x how far above the flop line the post is x the fatigue factor,
     * times the viral multiplier once Appeal crosses ViralAppeal; a sub-flop post
     * instead sheds followers. The following never drops below zero.
     */
    double Post(double Appeal);

    /**
     * Advance `Dt` hours: the audience cools, shedding DecayPerHour of the following
     * each idle hour, and the fatigue clock ticks up. A negative Dt is ignored.
     */
    void Advance(double Dt);

    /** Current follower count. */
    double Followers() const { return FollowerCount; }

    /** Current audience tier. */
    ETier Tier() const;

    /** Hours since the last Post (large until the first post, so the first post isn't fatigued). */
    double HoursSinceLastPost() const { return HoursRested; }

    /**
     * The reach a post would have right now before appeal/fatigue — BaseReach grown by
     * the current audience. Lets the adapter preview "this could reach ~N people".
     */
    double CurrentReach() const;

private:
    FParams Params;
    double FollowerCount = 0.0;
    double HoursRested = 0.0;
};
