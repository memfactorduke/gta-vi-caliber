// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TrafficSignal.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"
#include <initializer_list>

using GtcTest::Eps;

/**
 * Tests for FTrafficSignal — junction traffic lights. GTC-original (no Godot
 * oracle). Pins the phase ring (green→yellow→all-red→next phase), per-group
 * RED/YELLOW/GREEN resolution, the all-red clearance, groups never named in a
 * phase staying Red, the offset desync between junctions, the TimeUntilChange
 * countdown, cycle-length math, collapsed zero-duration phases, looping, the
 * dark (no-phase) junction, and negative-Dt clamping. Prefix GTC.AI.Traffic.Signal.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace TrafficSignalTest
{
    using FPhase = FTrafficSignal::FPhase;

    FPhase Phase(std::initializer_list<int32> Groups, double Green, double Yellow)
    {
        FPhase P;
        P.GreenSeconds = Green;
        P.YellowSeconds = Yellow;
        for (const int32 G : Groups)
        {
            P.GoGroups.Add(G);
        }
        return P;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTrafficSignalTest,
    "GTC.AI.Traffic.Signal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FTrafficSignalTest::RunTest(const FString& Parameters)
{
    using namespace TrafficSignalTest;
    using ESignal = FTrafficSignal::ESignal;

    // A classic two-phase crossroads: group 0 = N/S, group 1 = E/W.
    // NS: green 30 / yellow 5. EW: green 20 / yellow 5. All-red clearance 2 after each.
    // Cycle = (30+5+2) + (20+5+2) = 64.
    auto MakeCross = []()
    {
        TArray<FPhase> Phases;
        Phases.Add(Phase({0}, 30.0, 5.0));
        Phases.Add(Phase({1}, 20.0, 5.0));
        return Phases;
    };

    // ---- Fresh cross: NS green, EW red, cycle length, countdown -----------------
    {
        FTrafficSignal S;
        S.Configure(MakeCross(), /*allRed*/ 2.0);
        TestTrue(TEXT("cycle is 64s"), FMath::IsNearlyEqual(S.CycleSeconds(), 64.0, Eps));
        TestTrue(TEXT("NS starts green"), S.StateFor(0) == ESignal::Green);
        TestTrue(TEXT("EW starts red"), S.StateFor(1) == ESignal::Red);
        TestEqual(TEXT("active phase 0"), S.ActivePhase(), 0);
        TestFalse(TEXT("not all-red at start"), S.IsAllRed());
        TestTrue(TEXT("30s until NS yellow"), FMath::IsNearlyEqual(S.TimeUntilChange(), 30.0, Eps));
    }

    // ---- Green -> Yellow -> All-red -> next phase green -------------------------
    {
        FTrafficSignal S;
        S.Configure(MakeCross(), 2.0);

        S.Advance(10.0); // t=10, NS still green
        TestTrue(TEXT("NS green at t=10"), S.StateFor(0) == ESignal::Green);
        TestTrue(TEXT("20s left of NS green"), FMath::IsNearlyEqual(S.TimeUntilChange(), 20.0, Eps));

        S.Advance(22.0); // t=32, into NS yellow (30..35)
        TestTrue(TEXT("NS yellow at t=32"), S.StateFor(0) == ESignal::Yellow);
        TestTrue(TEXT("EW still red during NS yellow"), S.StateFor(1) == ESignal::Red);
        TestTrue(TEXT("3s left of NS yellow"), FMath::IsNearlyEqual(S.TimeUntilChange(), 3.0, Eps));

        S.Advance(4.0); // t=36, all-red clearance (35..37)
        TestTrue(TEXT("NS red in clearance"), S.StateFor(0) == ESignal::Red);
        TestTrue(TEXT("EW red in clearance"), S.StateFor(1) == ESignal::Red);
        TestTrue(TEXT("clearance is all-red"), S.IsAllRed());
        TestEqual(TEXT("clearance still belongs to phase 0"), S.ActivePhase(), 0);

        S.Advance(2.0); // t=38, EW green (37..57)
        TestTrue(TEXT("EW green at t=38"), S.StateFor(1) == ESignal::Green);
        TestTrue(TEXT("NS red while EW green"), S.StateFor(0) == ESignal::Red);
        TestEqual(TEXT("active phase 1"), S.ActivePhase(), 1);
        TestFalse(TEXT("not all-red during EW green"), S.IsAllRed());

        S.Advance(22.0); // t=60, EW yellow (57..62)
        TestTrue(TEXT("EW yellow at t=60"), S.StateFor(1) == ESignal::Yellow);

        S.Advance(3.0); // t=63, final all-red (62..64)
        TestTrue(TEXT("final clearance all-red"), S.IsAllRed());
        TestEqual(TEXT("final clearance belongs to phase 1"), S.ActivePhase(), 1);

        S.Advance(1.0); // t=64 -> wraps to cycle start, NS green again
        TestTrue(TEXT("NS green after full cycle"), S.StateFor(0) == ESignal::Green);
        TestTrue(TEXT("EW red after full cycle"), S.StateFor(1) == ESignal::Red);
    }

    // ---- A group never named in any phase stays Red forever --------------------
    {
        FTrafficSignal S;
        S.Configure(MakeCross(), 2.0);
        TestTrue(TEXT("unknown group red at t=0"), S.StateFor(99) == ESignal::Red);
        S.Advance(40.0);
        TestTrue(TEXT("unknown group red mid-cycle"), S.StateFor(99) == ESignal::Red);
    }

    // ---- Offset desyncs two identical junctions --------------------------------
    {
        FTrafficSignal A, B;
        A.Configure(MakeCross(), 2.0, /*offset*/ 0.0);
        B.Configure(MakeCross(), 2.0, /*offset*/ 37.0); // 37s in => EW green
        TestTrue(TEXT("A NS green at t=0"), A.StateFor(0) == ESignal::Green);
        TestTrue(TEXT("B EW green at t=0 (offset)"), B.StateFor(1) == ESignal::Green);
        TestTrue(TEXT("B NS red at t=0 (offset)"), B.StateFor(0) == ESignal::Red);
        TestNotEqual(TEXT("the junctions are desynced"),
            (int32)A.ActivePhase(), (int32)B.ActivePhase());
    }

    // ---- A collapsed zero-duration phase is skipped instantaneously ------------
    {
        FTrafficSignal S;
        TArray<FPhase> Phases;
        Phases.Add(Phase({0}, 0.0, 0.0));  // collapses to nothing
        Phases.Add(Phase({1}, 10.0, 0.0)); // the only phase with airtime
        S.Configure(MoveTemp(Phases), /*allRed*/ 0.0);
        TestTrue(TEXT("collapsed-phase cycle is 10s"), FMath::IsNearlyEqual(S.CycleSeconds(), 10.0, Eps));
        TestTrue(TEXT("group 1 green at t=0"), S.StateFor(1) == ESignal::Green);
        TestTrue(TEXT("collapsed group 0 never shows"), S.StateFor(0) == ESignal::Red);
        TestEqual(TEXT("active phase skips to 1"), S.ActivePhase(), 1);
        S.Advance(10.0); // wraps; phase 0 still collapsed, so back to phase 1 green
        TestTrue(TEXT("group 1 green again after loop"), S.StateFor(1) == ESignal::Green);
    }

    // ---- A dark junction (no phases) is all-red and reports no active phase -----
    {
        FTrafficSignal S;
        S.Configure({});
        TestTrue(TEXT("dark junction has zero cycle"), FMath::Abs(S.CycleSeconds()) <= Eps);
        TestTrue(TEXT("dark junction is red"), S.StateFor(0) == ESignal::Red);
        TestEqual(TEXT("dark junction has no active phase"), S.ActivePhase(), INDEX_NONE);
        TestFalse(TEXT("dark junction not flagged all-red"), S.IsAllRed());
        TestTrue(TEXT("dark junction countdown is 0"), FMath::Abs(S.TimeUntilChange()) <= Eps);
        S.Advance(100.0); // must not crash or divide by zero
        TestTrue(TEXT("dark junction still red"), S.StateFor(0) == ESignal::Red);
    }

    // ---- A negative Dt never runs the lights backwards -------------------------
    {
        FTrafficSignal S;
        S.Configure(MakeCross(), 2.0);
        S.Advance(10.0);
        S.Advance(-5.0); // ignored
        TestTrue(TEXT("negative Dt ignored"), FMath::IsNearlyEqual(S.Clock(), 10.0, Eps));
        TestTrue(TEXT("state unchanged by negative Dt"), S.StateFor(0) == ESignal::Green);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
