// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WildlifeBehavior.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FWildlifeBehavior — the temperament-branched fauna reflex. GTC-original
 * (no Godot oracle). Pins the Idle->Alert notice, the per-temperament escalation at
 * ReactRange (Skittish flees, Aggressive charges, Placid holds), provoke forcing an
 * escalation at any distance, the calm-down hysteresis + CalmProgress, the ReactRange
 * clamp to NoticeRange, negative-distance/Dt clamping, and the empty-threat idle.
 * Prefix GTC.NPC.Wildlife.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace WildlifeBehaviorTest
{
    using FParams = FWildlifeBehavior::FParams;
    using ETemperament = FWildlifeBehavior::ETemperament;

    FParams Tuning(ETemperament Temperament)
    {
        FParams P;
        P.Temperament = Temperament;
        P.NoticeRange = 1500.0;
        P.ReactRange = 600.0;
        P.CalmSeconds = 4.0;
        return P;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWildlifeBehaviorTest,
    "GTC.NPC.Wildlife",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FWildlifeBehaviorTest::RunTest(const FString& Parameters)
{
    using namespace WildlifeBehaviorTest;
    using EState = FWildlifeBehavior::EState;

    // ---- Fresh animal idles; a far threat doesn't register --------------------
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Skittish);
        TestTrue(TEXT("fresh animal idles"), A.State() == EState::Idle);
        A.Update(/*dist*/ 2000.0, /*provoked*/ false, 0.1, P); // beyond NoticeRange
        TestTrue(TEXT("far threat keeps it idle"), A.State() == EState::Idle);
        TestTrue(TEXT("idle calm progress is 0"), FMath::Abs(A.CalmProgress(P)) <= Eps);
    }

    // ---- Skittish: Idle -> Alert at notice -> Flee in personal space -----------
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Skittish);
        A.Update(1000.0, false, 0.1, P); // inside notice, outside react
        TestTrue(TEXT("skittish goes alert"), A.State() == EState::Alert);
        A.Update(500.0, false, 0.1, P); // inside react
        TestTrue(TEXT("skittish flees when crowded"), A.State() == EState::Flee);
        TestTrue(TEXT("IsFleeing reflects Flee"), A.IsFleeing());
    }

    // ---- Aggressive: Alert -> Charge in personal space ------------------------
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Aggressive);
        A.Update(1000.0, false, 0.1, P);
        TestTrue(TEXT("aggressive goes alert"), A.State() == EState::Alert);
        A.Update(500.0, false, 0.1, P);
        TestTrue(TEXT("aggressive charges when crowded"), A.State() == EState::Charge);
        TestTrue(TEXT("IsHostile reflects Charge"), A.IsHostile());
    }

    // ---- Placid: notices but never escalates on its own -----------------------
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Placid);
        A.Update(1000.0, false, 0.1, P);
        TestTrue(TEXT("placid goes alert"), A.State() == EState::Alert);
        A.Update(100.0, false, 0.1, P); // right on top of it
        TestTrue(TEXT("placid stays alert even when crowded"), A.State() == EState::Alert);
        TestFalse(TEXT("placid never hostile"), A.IsHostile());
        TestFalse(TEXT("placid never flees unprovoked"), A.IsFleeing());
    }

    // ---- A provoke forces escalation at ANY distance, by temperament ----------
    {
        FWildlifeBehavior Skit, Aggr, Plac;
        Skit.Update(9999.0, /*provoked*/ true, 0.1, Tuning(ETemperament::Skittish));
        Aggr.Update(9999.0, true, 0.1, Tuning(ETemperament::Aggressive));
        Plac.Update(9999.0, true, 0.1, Tuning(ETemperament::Placid));
        TestTrue(TEXT("provoked skittish flees from afar"), Skit.State() == EState::Flee);
        TestTrue(TEXT("provoked aggressive charges from afar"), Aggr.State() == EState::Charge);
        TestTrue(TEXT("provoked placid flees from afar"), Plac.State() == EState::Flee);
    }

    // ---- Calm-down: stays reactive until the threat has been gone CalmSeconds --
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Skittish); // CalmSeconds = 4
        A.Update(1000.0, false, 0.1, P);
        A.Update(500.0, false, 0.1, P);
        TestTrue(TEXT("fleeing"), A.State() == EState::Flee);

        A.Update(2000.0, false, 2.0, P); // threat gone, 2s < 4s
        TestTrue(TEXT("still fleeing before calm window"), A.State() == EState::Flee);
        TestTrue(TEXT("calm progress half way"), FMath::IsNearlyEqual(A.CalmProgress(P), 0.5, Eps));

        A.Update(2000.0, false, 2.0, P); // 4s elapsed beyond notice -> settle
        TestTrue(TEXT("settles to idle after calm window"), A.State() == EState::Idle);
        TestTrue(TEXT("idle resets calm progress"), FMath::Abs(A.CalmProgress(P)) <= Eps);
    }

    // ---- Threat re-entering the notice ring resets the calm-down ---------------
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Skittish);
        A.Update(1000.0, false, 0.1, P);
        A.Update(500.0, false, 0.1, P); // Flee
        A.Update(2000.0, false, 3.0, P); // 3s gone
        A.Update(1000.0, false, 0.1, P); // back inside notice -> resets timer, still Flee
        TestTrue(TEXT("re-entry keeps it fleeing"), A.State() == EState::Flee);
        TestTrue(TEXT("re-entry reset calm progress"), FMath::Abs(A.CalmProgress(P)) <= Eps);
    }

    // ---- ReactRange is clamped to NoticeRange (can't react before noticing) ----
    {
        FWildlifeBehavior A;
        FParams P = Tuning(ETemperament::Skittish);
        P.ReactRange = 5000.0; // absurd; clamps to NoticeRange (1500)
        A.Update(1400.0, false, 0.1, P); // inside notice -> Alert
        TestTrue(TEXT("alert at clamped edge"), A.State() == EState::Alert);
        A.Update(1500.0, false, 0.1, P); // distance == NoticeRange == clamped react -> Flee
        TestTrue(TEXT("flees at the clamped react edge"), A.State() == EState::Flee);
    }

    // ---- Negative distance is on-top-of (reacts); negative Dt never advances calm
    {
        FWildlifeBehavior A;
        const FParams P = Tuning(ETemperament::Aggressive);
        A.Update(1000.0, false, 0.1, P);
        A.Update(-50.0, false, 0.1, P); // negative distance -> 0 -> inside react -> charge
        TestTrue(TEXT("negative distance is point-blank -> charge"), A.State() == EState::Charge);

        A.Update(2000.0, false, -5.0, P); // threat gone but negative Dt adds no calm time
        TestTrue(TEXT("negative Dt keeps it charging"), A.State() == EState::Charge);
        TestTrue(TEXT("negative Dt added no calm progress"), FMath::Abs(A.CalmProgress(P)) <= Eps);
    }

    // ---- CalmSeconds == 0 settles the moment the threat is gone ----------------
    {
        FWildlifeBehavior A;
        FParams P = Tuning(ETemperament::Skittish);
        P.CalmSeconds = 0.0;
        A.Update(1000.0, false, 0.1, P);
        A.Update(500.0, false, 0.1, P); // Flee
        TestTrue(TEXT("zero-calm flee progress is 1"), FMath::IsNearlyEqual(A.CalmProgress(P), 1.0, Eps));
        A.Update(2000.0, false, 0.0, P); // threat gone, no delay
        TestTrue(TEXT("zero-calm settles immediately"), A.State() == EState::Idle);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
