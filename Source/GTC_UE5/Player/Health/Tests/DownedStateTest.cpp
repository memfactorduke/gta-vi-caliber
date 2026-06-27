// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../DownedState.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FDownedState — the bleed-out / revive grace window. GTC-original (no
 * Godot oracle). Pins GoDown, the bleed-out clock + progress/time-left, Revive,
 * FinishOff, the downs budget (instant kill once spent), Reset, the no-op guards
 * (re-down/revive/tick in the wrong state), and negative-Dt clamping. Prefix
 * GTC.Player.Downed. All inputs are method calls, so no shared helpers.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDownedStateTest,
    "GTC.Player.Downed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDownedStateTest::RunTest(const FString& Parameters)
{
    using EState = FDownedState::EState;
    const FDownedState::FParams P; // bleed-out 20s, max downs 2

    // ---- Fresh player is up ---------------------------------------------------
    {
        FDownedState D;
        D.Configure(P);
        TestTrue(TEXT("fresh is up"), D.IsUp());
        TestFalse(TEXT("not downed"), D.IsDowned());
        TestFalse(TEXT("not dead"), D.IsDead());
        TestEqual(TEXT("no downs taken"), D.DownsTaken(), 0);
        TestTrue(TEXT("no bleed progress while up"), FMath::Abs(D.BleedProgress()) <= Eps);
    }

    // ---- Going down starts the bleed-out clock --------------------------------
    {
        FDownedState D;
        D.Configure(P);
        D.GoDown();
        TestTrue(TEXT("now downed"), D.IsDowned());
        TestEqual(TEXT("one down spent"), D.DownsTaken(), 1);
        TestTrue(TEXT("full bleed window left"), FMath::IsNearlyEqual(D.TimeLeft(), 20.0, Eps));
        TestTrue(TEXT("progress 0 at the drop"), FMath::Abs(D.BleedProgress()) <= Eps);

        D.Tick(10.0);
        TestTrue(TEXT("halfway bled out"), FMath::IsNearlyEqual(D.BleedProgress(), 0.5, Eps));
        TestTrue(TEXT("10s left"), FMath::IsNearlyEqual(D.TimeLeft(), 10.0, Eps));
        TestTrue(TEXT("still downed at half"), D.IsDowned());

        D.Tick(10.0); // reach the bleed-out
        TestTrue(TEXT("bled out -> dead"), D.IsDead());
        TestTrue(TEXT("no progress reported once dead"), FMath::Abs(D.BleedProgress()) <= Eps);
    }

    // ---- A revive pulls you back up; the clock resets --------------------------
    {
        FDownedState D;
        D.Configure(P);
        D.GoDown();
        D.Tick(5.0);
        D.Revive();
        TestTrue(TEXT("revived to up"), D.IsUp());
        TestTrue(TEXT("no time left while up"), FMath::Abs(D.TimeLeft()) <= Eps);

        D.GoDown(); // down again
        TestTrue(TEXT("downed again"), D.IsDowned());
        TestTrue(TEXT("clock reset on the new down"), FMath::IsNearlyEqual(D.TimeLeft(), 20.0, Eps));
        TestEqual(TEXT("two downs spent now"), D.DownsTaken(), 2);
    }

    // ---- FinishOff executes a downed player -----------------------------------
    {
        FDownedState D;
        D.Configure(P);
        D.FinishOff(); // up -> no-op
        TestTrue(TEXT("can't finish off someone who's up"), D.IsUp());
        D.GoDown();
        D.FinishOff();
        TestTrue(TEXT("finished off -> dead"), D.IsDead());
    }

    // ---- The downs budget runs out: the next drop is an instant kill -----------
    {
        FDownedState D;
        D.Configure(P); // MaxDowns 2
        D.GoDown(); D.Revive(); // 1
        D.GoDown(); D.Revive(); // 2
        TestEqual(TEXT("two downs survived"), D.DownsTaken(), 2);
        D.GoDown(); // 3rd drop, budget spent
        TestTrue(TEXT("out of chances -> instant dead"), D.IsDead());
        TestEqual(TEXT("the fatal drop didn't spend a down"), D.DownsTaken(), 2);
    }

    // ---- Reset restores standing and the budget -------------------------------
    {
        FDownedState D;
        D.Configure(P);
        D.GoDown();
        D.Tick(25.0); // dead
        TestTrue(TEXT("dead before reset"), D.IsDead());
        D.Reset();
        TestTrue(TEXT("reset to up"), D.IsUp());
        TestEqual(TEXT("downs budget restored"), D.DownsTaken(), 0);
        D.GoDown();
        TestTrue(TEXT("can go down again after reset"), D.IsDowned());
    }

    // ---- No-op guards: wrong-state Revive/Tick, re-down, negative Dt -----------
    {
        FDownedState D;
        D.Configure(P);
        D.Revive(); // up -> no-op
        TestTrue(TEXT("reviving someone who's up does nothing"), D.IsUp());
        D.Tick(5.0); // up -> no-op
        TestTrue(TEXT("ticking while up does nothing"), D.IsUp());

        D.GoDown();
        D.Tick(5.0);
        D.GoDown(); // already downed -> must NOT reset the clock or re-spend a down
        TestTrue(TEXT("re-downing doesn't reset the clock"), FMath::IsNearlyEqual(D.TimeLeft(), 15.0, Eps));
        TestEqual(TEXT("re-downing doesn't spend another down"), D.DownsTaken(), 1);

        const double Left = D.TimeLeft();
        D.Tick(-5.0); // negative -> no advance
        TestTrue(TEXT("negative Dt doesn't advance the clock"), FMath::IsNearlyEqual(D.TimeLeft(), Left, Eps));

        // Dead is terminal until Reset.
        D.Tick(100.0);
        TestTrue(TEXT("now dead"), D.IsDead());
        D.Revive();
        TestTrue(TEXT("can't revive the dead"), D.IsDead());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
