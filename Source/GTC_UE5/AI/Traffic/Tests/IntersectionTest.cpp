// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Intersection.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FIntersection — right-of-way arbitration. GTC-original. Pins the
 * priority order (major road > FCFS arrival > lower approach id) and the
 * stop-line gap a yielding car feeds FTrafficModel.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIntersectionTest,
    "GTC.AI.Traffic.Intersection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIntersectionTest::RunTest(const FString& Parameters)
{
    using FApproach = FIntersection::FApproach;

    // Major road beats a minor approach arriving at the same time.
    {
        TArray<FApproach> Claims;
        Claims.Add(FApproach{0, 5.0, 1}); // major
        Claims.Add(FApproach{1, 5.0, 0}); // minor
        TestEqual(TEXT("major road has right-of-way"), FIntersection::RightOfWay(Claims), 0);
        TestTrue(TEXT("minor yields to major"), FIntersection::Yields(Claims[1], Claims[0]));
        TestFalse(TEXT("major does not yield"), FIntersection::Yields(Claims[0], Claims[1]));
        TestTrue(TEXT("major may proceed"), FIntersection::MayProceed(Claims, 0));
        TestFalse(TEXT("minor must wait"), FIntersection::MayProceed(Claims, 1));
    }

    // All-way stop (equal priority): first to the line goes first.
    {
        TArray<FApproach> Claims;
        Claims.Add(FApproach{0, 8.0, 0});
        Claims.Add(FApproach{1, 3.0, 0}); // earliest
        Claims.Add(FApproach{2, 5.0, 0});
        TestEqual(TEXT("earliest arrival has right-of-way"), FIntersection::RightOfWay(Claims), 1);
    }

    // Exact tie on priority + arrival -> lowest approach id (yield to the right).
    {
        TArray<FApproach> Claims;
        Claims.Add(FApproach{2, 4.0, 0});
        Claims.Add(FApproach{0, 4.0, 0}); // lowest approach id
        Claims.Add(FApproach{1, 4.0, 0});
        TestEqual(TEXT("tie breaks to lowest approach id"), FIntersection::RightOfWay(Claims), 1);
    }

    // Empty / out-of-range are safe.
    TestEqual(TEXT("no claims -> none"), FIntersection::RightOfWay(TArray<FApproach>()), -1);
    TestFalse(TEXT("out-of-range may not proceed"), FIntersection::MayProceed(TArray<FApproach>(), 0));

    // Stop line behaves like a stopped leader.
    TestTrue(TEXT("stop-line gap nets front half"), FMath::Abs(FIntersection::StopLineGap(8.0, 2.25) - 5.75) <= Eps);
    TestTrue(TEXT("stop-line gap clamps at the line"), FMath::Abs(FIntersection::StopLineGap(1.0, 2.25)) <= Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
