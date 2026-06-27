// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CloutLedger.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FCloutLedger — the social-media following. GTC-original (no Godot
 * oracle). Pins the first-post gain, the network effect (more audience -> more
 * reach), the viral multiplier (and that it's continuous at the threshold), the
 * flop loss, posting fatigue + recovery, idle audience decay, the follower floor
 * at zero, appeal clamping, tier thresholds, and the HoursSinceLastPost clock.
 * Prefix GTC.Systems.Clout.
 *
 * Clean single-post values are asserted tightly; composite (audience/fatigue)
 * effects are asserted as inequalities so they don't depend on hand arithmetic.
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace CloutLedgerTest
{
    using FParams = FCloutLedger::FParams;

    // The documented defaults, restated so the expected numbers are explicit.
    FParams Defaults()
    {
        FParams P; // BaseReach 200, AudienceFactor 0.05, FlopAppeal 0.25, FlopLoss 0.5,
                   // ViralAppeal 0.8, ViralMultiplier 5, FatigueWindow 3, FatigueFloor 0.15,
                   // DecayPerHour 0.01, tiers 1000/25000/250000/1000000
        return P;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCloutLedgerTest,
    "GTC.Systems.Clout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCloutLedgerTest::RunTest(const FString& Parameters)
{
    using namespace CloutLedgerTest;
    using ETier = FCloutLedger::ETier;

    // ---- Fresh ledger: nobody, Unknown, rested --------------------------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        TestTrue(TEXT("fresh has no followers"), FMath::Abs(C.Followers()) <= Eps);
        TestTrue(TEXT("fresh is Unknown"), C.Tier() == ETier::Unknown);
        TestTrue(TEXT("fresh is rested (HoursSinceLastPost == window)"),
            FMath::IsNearlyEqual(C.HoursSinceLastPost(), 3.0, Eps));
        TestTrue(TEXT("fresh reach is BaseReach"), FMath::IsNearlyEqual(C.CurrentReach(), 200.0, Eps));
    }

    // ---- First good post: reach 200 x quality, rested, not viral ---------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        const double D = C.Post(0.5); // quality = (0.5-0.25)/0.75 = 1/3 -> 66.6667
        TestTrue(TEXT("first post gains ~66.67"), FMath::IsNearlyEqual(D, 200.0 / 3.0, 1e-3));
        TestTrue(TEXT("followers match the gain"), FMath::IsNearlyEqual(C.Followers(), 200.0 / 3.0, 1e-3));
        TestTrue(TEXT("posting resets the rest clock"), FMath::Abs(C.HoursSinceLastPost()) <= Eps);
    }

    // ---- Viral: appeal 1 multiplies; threshold is continuous (no jump) ---------
    {
        FCloutLedger Big;
        Big.Configure(Defaults());
        const double DViral = Big.Post(1.0); // quality 1, viral x5 -> 200*1*5 = 1000
        TestTrue(TEXT("a perfect post goes viral to ~1000"), FMath::IsNearlyEqual(DViral, 1000.0, 1e-2));

        FCloutLedger AtLine;
        AtLine.Configure(Defaults());
        const double DLine = AtLine.Post(0.8); // exactly at ViralAppeal -> viral 1x, quality 0.7333 -> 146.67
        TestTrue(TEXT("at the viral line there's no jump (~146.67)"),
            FMath::IsNearlyEqual(DLine, 200.0 * (0.55 / 0.75), 1e-2));
        TestTrue(TEXT("viral post dwarfs the threshold post"), DViral > DLine * 5.0);
    }

    // ---- The network effect: same post earns more with a bigger audience ------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        const double First = C.Post(0.6);
        C.Advance(3.0); // rest back to full so only the audience differs
        const double Second = C.Post(0.6);
        TestTrue(TEXT("rested repost with more followers earns more"), Second > First);
    }

    // ---- Fatigue: a back-to-back post earns far less, then recovers ------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        const double Rested = C.Post(0.6);
        const double Spammed = C.Post(0.6); // HoursRested == 0 -> fatigue floor
        TestTrue(TEXT("spammed post earns less than the rested one"), Spammed < Rested);

        C.Advance(1.5); // half the window
        const double HalfRested = C.Post(0.6);
        TestTrue(TEXT("half-rested earns more than fully spammed"), HalfRested > Spammed);
    }

    // ---- A flop loses followers; the count floors at zero ----------------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        C.Post(1.0); // build an audience first
        const double Before = C.Followers();
        const double D = C.Post(0.1); // below FlopAppeal -> a flop
        TestTrue(TEXT("a flop is a net loss"), D < 0.0);
        TestTrue(TEXT("a flop sheds followers"), C.Followers() < Before);

        FCloutLedger Z;
        Z.Configure(Defaults());
        const double DZ = Z.Post(0.0); // worst flop from zero -> -0.5*200 = -100, clamped to 0
        TestTrue(TEXT("worst flop computes -100"), FMath::IsNearlyEqual(DZ, -100.0, 1e-2));
        TestTrue(TEXT("followers never go negative"), FMath::Abs(Z.Followers()) <= Eps);
    }

    // ---- Idle decay: the audience cools while you're offline -------------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        C.Post(1.0); // ~1000 followers
        const double Peak = C.Followers();
        C.Advance(10.0); // shed 10% (0.01/hr * 10h)
        TestTrue(TEXT("10h idle sheds ~10%"), FMath::IsNearlyEqual(C.Followers(), Peak * 0.9, 1.0));
        TestTrue(TEXT("rest clock advanced 10h"), FMath::IsNearlyEqual(C.HoursSinceLastPost(), 10.0, Eps));

        C.Advance(100000.0); // huge idle -> decay caps, never negative
        TestTrue(TEXT("massive idle floors at zero, not negative"), FMath::Abs(C.Followers()) <= 1.0);
        TestTrue(TEXT("followers stay >= 0"), C.Followers() >= 0.0);
    }

    // ---- Negative Dt is ignored ----------------------------------------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        C.Post(0.7);
        const double F = C.Followers();
        const double R = C.HoursSinceLastPost();
        C.Advance(-5.0);
        TestTrue(TEXT("negative Dt doesn't change followers"), FMath::IsNearlyEqual(C.Followers(), F, Eps));
        TestTrue(TEXT("negative Dt doesn't advance the rest clock"),
            FMath::IsNearlyEqual(C.HoursSinceLastPost(), R, Eps));
    }

    // ---- Appeal is clamped to [0,1] -------------------------------------------
    {
        FCloutLedger Over;
        Over.Configure(Defaults());
        const double DOver = Over.Post(5.0); // clamps to 1.0 -> viral 1000

        FCloutLedger One;
        One.Configure(Defaults());
        const double DOne = One.Post(1.0);
        TestTrue(TEXT("over-range appeal clamps to 1"), FMath::IsNearlyEqual(DOver, DOne, Eps));

        FCloutLedger Under;
        Under.Configure(Defaults());
        const double DUnder = Under.Post(-3.0); // clamps to 0 -> worst flop -100
        TestTrue(TEXT("under-range appeal clamps to 0"), FMath::IsNearlyEqual(DUnder, -100.0, 1e-2));
    }

    // ---- Tiers: a single viral post lifts Unknown -> Nano ---------------------
    {
        FCloutLedger C;
        C.Configure(Defaults());
        TestTrue(TEXT("starts Unknown"), C.Tier() == ETier::Unknown);
        C.Post(1.0); // ~1000 = NanoAt
        TestTrue(TEXT("a viral post reaches Nano"), C.Tier() == ETier::Nano);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
