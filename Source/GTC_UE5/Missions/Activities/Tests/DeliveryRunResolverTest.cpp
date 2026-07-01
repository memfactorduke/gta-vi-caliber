// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../DeliveryRunResolver.h"
#include "../DeliveryRun.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FDeliveryRunResolver — cooking world state (distance-to-drop + crash impulse) into
 * FDeliveryRun's ProgressDelta/Damage, with a monotonic high-water route so backtracking can't
 * double-count. Also drives a couple of full runs through the real FDeliveryRun end-to-end.
 * Prefix GTC.Missions.Activities.DeliveryResolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDeliveryRunResolverTest,
    "GTC.Missions.Activities.DeliveryResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FDeliveryRunResolverTest::RunTest(const FString& Parameters)
{
    // Radius defaults to 0 for the pure-fraction cases (progress hits 1.0 only exactly on the drop);
    // the arrival-radius cases pass it explicitly.
    auto Cook = [&](double Dist, double Route, double Prev, double Impulse, double Radius = 0.0)
    {
        FDeliveryRunResolver::FParams PP;
        PP.ArrivalRadius = Radius;
        FDeliveryRunResolver::FInput In;
        In.DistanceToDropoff = Dist;
        In.RouteLength = Route;
        In.PrevProgress = Prev;
        In.ImpactImpulse = Impulse;
        return FDeliveryRunResolver::Cook(In, PP);
    };

    // --- Progress fraction: start of a 1000-unit route is 0, halfway is 0.5, the drop is 1. ---
    {
        TestTrue(TEXT("at the start -> 0 progress"), FMath::IsNearlyEqual(Cook(1000, 1000, 0, 0).Progress, 0.0, Eps));
        const FDeliveryRunResolver::FOutput Half = Cook(500, 1000, 0, 0);
        TestTrue(TEXT("halfway -> 0.5 progress"), FMath::IsNearlyEqual(Half.Progress, 0.5, Eps));
        TestTrue(TEXT("halfway from 0 -> 0.5 delta"), FMath::IsNearlyEqual(Half.ProgressDelta, 0.5, Eps));
        TestTrue(TEXT("at the drop -> full progress"), FMath::IsNearlyEqual(Cook(0, 1000, 0, 0).Progress, 1.0, Eps));
    }

    // --- Monotonic high-water: driving away then back does NOT re-award the route. ---
    {
        const FDeliveryRunResolver::FOutput Away = Cook(1000, 1000, 0.5, 0); // back at the start, high-water 0.5
        TestTrue(TEXT("backtracking holds the high-water progress"), FMath::IsNearlyEqual(Away.Progress, 0.5, Eps));
        TestTrue(TEXT("backtracking awards no new progress"), FMath::IsNearlyEqual(Away.ProgressDelta, 0.0, Eps));
        const FDeliveryRunResolver::FOutput Back = Cook(500, 1000, 0.5, 0); // returned to halfway, already banked
        TestTrue(TEXT("re-covering old ground awards no delta"), FMath::IsNearlyEqual(Back.ProgressDelta, 0.0, Eps));
        const FDeliveryRunResolver::FOutput New = Cook(250, 1000, 0.5, 0); // past the prior best
        TestTrue(TEXT("only new ground past the high-water counts"), FMath::IsNearlyEqual(New.ProgressDelta, 0.25, Eps));
    }

    // --- Degenerate routes are safe. ---
    {
        TestTrue(TEXT("zero-length route is already delivered"), FMath::IsNearlyEqual(Cook(0, 0, 0, 0).Progress, 1.0, Eps));
        TestTrue(TEXT("driving past the start clamps to 0, never negative delta"),
            FMath::IsNearlyEqual(Cook(2000, 1000, 0, 0).ProgressDelta, 0.0, Eps));
    }

    // --- Arrival radius: progress reaches 1.0 near (not exactly on) the drop, and ramps to it. ---
    {
        // 500 cm radius on a 1000-unit route: arriving within 500 cm of the drop delivers.
        TestTrue(TEXT("within the arrival radius -> full progress"),
            FMath::IsNearlyEqual(Cook(400, 1000, 0, 0, 500).Progress, 1.0, Eps));
        TestTrue(TEXT("exactly at the radius edge -> full progress"),
            FMath::IsNearlyEqual(Cook(500, 1000, 0, 0, 500).Progress, 1.0, Eps));
        // Halfway of the REACHABLE span (route - radius): dist = radius + 0.5*(route-radius) = 750.
        TestTrue(TEXT("progress ramps smoothly to 1.0 at the radius (750 -> 0.5)"),
            FMath::IsNearlyEqual(Cook(750, 1000, 0, 0, 500).Progress, 0.5, Eps));
        TestTrue(TEXT("a route shorter than the radius is already there"),
            FMath::IsNearlyEqual(Cook(300, 400, 0, 0, 500).Progress, 1.0, Eps));
    }

    // --- Cargo damage: impulse scaled by DamagePerImpulse; negatives ignored. ---
    {
        TestTrue(TEXT("impulse -> proportional cargo damage"),
            FMath::IsNearlyEqual(Cook(500, 1000, 0, 5000).Damage, 5000.0 * 0.0002, Eps));
        TestTrue(TEXT("no impact -> no damage"), FMath::IsNearlyEqual(Cook(500, 1000, 0, 0).Damage, 0.0, Eps));
        TestTrue(TEXT("negative impulse ignored"), FMath::IsNearlyEqual(Cook(500, 1000, 0, -100).Damage, 0.0, Eps));
    }

    // --- End-to-end: a clean run delivers with a healthy payout. ---
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 100.0;
        D.Configure(DP);
        double High = 0.0;
        // Close the 1000-unit route over 10 ticks of 1s, no crashes.
        for (int32 i = 1; i <= 10; ++i)
        {
            const double Dist = 1000.0 - i * 100.0;
            const FDeliveryRunResolver::FOutput C = Cook(Dist, 1000, High, 0);
            High = C.Progress;
            D.Update(C.ProgressDelta, C.Damage, 1.0);
        }
        TestTrue(TEXT("clean run delivers"), D.IsDelivered());
        TestTrue(TEXT("clean run pays well"), D.PayoutFactor() > 0.8);
    }

    // --- End-to-end with the arrival radius: stopping NEAR the drop delivers (not exactly on it). ---
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 100.0;
        D.Configure(DP);
        double High = 0.0;
        // Drive a 1000-unit route with a 500 cm radius, stopping 400 cm short of the marker.
        for (int32 i = 1; i <= 6; ++i)
        {
            const double Dist = FMath::Max(400.0, 1000.0 - i * 100.0);
            const FDeliveryRunResolver::FOutput C = Cook(Dist, 1000, High, 0, 500);
            High = C.Progress;
            D.Update(C.ProgressDelta, C.Damage, 1.0);
        }
        TestTrue(TEXT("stopping within the arrival radius delivers"), D.IsDelivered());
    }

    // --- End-to-end: backtracking never falsely delivers (regression for the high-water rule). ---
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 1000.0;
        D.Configure(DP);
        double High = 0.0;
        // Oscillate start<->halfway forever: without the high-water rule this would accumulate to 1.
        for (int32 i = 0; i < 20; ++i)
        {
            const double Dist = (i % 2 == 0) ? 500.0 : 1000.0;
            const FDeliveryRunResolver::FOutput C = Cook(Dist, 1000, High, 0);
            High = C.Progress;
            D.Update(C.ProgressDelta, C.Damage, 1.0);
        }
        TestFalse(TEXT("oscillating short of the drop never delivers"), D.IsDelivered());
        TestTrue(TEXT("progress capped at the furthest reached (0.5)"), FMath::IsNearlyEqual(D.Progress(), 0.5, Eps));
    }

    // --- End-to-end: a big crash wrecks the cargo and fails the run. ---
    {
        FDeliveryRun D;
        FDeliveryRun::FParams DP;
        DP.TimeLimitSeconds = 100.0;
        D.Configure(DP);
        const FDeliveryRunResolver::FOutput C = Cook(900, 1000, 0, 100000); // huge impulse
        D.Update(C.ProgressDelta, C.Damage, 1.0);
        TestTrue(TEXT("a massive crash wrecks the cargo"), D.IsWrecked());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
