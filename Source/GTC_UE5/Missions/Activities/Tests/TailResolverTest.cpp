// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../TailResolver.h"
#include "../TailObjective.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FTailResolver — the distance + forward-view-cone geometry the live
 * UGTCTailObjectiveComponent feeds to FTailObjective, plus a couple of full tails driven
 * end-to-end through the real core. Prefix GTC.Missions.Activities.TailResolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FTailResolverTest,
    "GTC.Missions.Activities.TailResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FTailResolver::FOutput Cook(const FVector& Follower, const FVector& Target, const FVector& Fwd)
{
    FTailResolver::FParams P; // 45 deg half-angle, 4000 cm range
    FTailResolver::FInput In;
    In.FollowerPos = Follower;
    In.TargetPos = Target;
    In.TargetForward = Fwd;
    return FTailResolver::Cook(In, P);
}

bool FTailResolverTest::RunTest(const FString& Parameters)
{
    const FVector O(0, 0, 0);
    const FVector FwdX(1, 0, 0);

    // --- View cone. ---
    TestTrue(TEXT("directly in front, in range -> in view"), Cook(FVector(2000, 0, 0), O, FwdX).bInView);
    TestFalse(TEXT("behind the mark -> not in view"), Cook(FVector(-2000, 0, 0), O, FwdX).bInView);
    TestFalse(TEXT("90 deg to the side -> not in view"), Cook(FVector(0, 2000, 0), O, FwdX).bInView);
    TestTrue(TEXT("30 deg off (< 45 half-angle) -> in view"), Cook(FVector(1732, 1000, 0), O, FwdX).bInView);
    TestFalse(TEXT("in front but beyond range -> not in view"), Cook(FVector(5000, 0, 0), O, FwdX).bInView);
    TestTrue(TEXT("right on top of the mark -> in view"), Cook(O, O, FwdX).bInView);
    TestTrue(TEXT("distance is the follower->mark length"),
        FMath::IsNearlyEqual(Cook(FVector(1500, 0, 0), O, FwdX).Distance, 1500.0, Eps));

    // Ground-plane (yaw-only): a ramp-pitched facing still sees dead ahead; height drops from distance.
    TestTrue(TEXT("a mark facing up a ramp still sees a follower dead ahead"),
        Cook(FVector(2000, 0, 0), O, FVector(1, 0, 2)).bInView);
    TestTrue(TEXT("height is dropped from the tail distance"),
        FMath::IsNearlyEqual(Cook(FVector(1500, 0, 5000), O, FwdX).Distance, 1500.0, Eps));

    // --- Ride the band out of view -> succeed. ---
    {
        FTailObjective T;
        FTailObjective::FParams TP;
        T.Configure(TP);
        for (int32 i = 0; i < 20; ++i)
        {
            const FTailResolver::FOutput C = Cook(FVector(-1500, 0, 0), O, FwdX);
            T.Update(C.Distance, C.bInView, 1.0);
        }
        TestTrue(TEXT("riding the band out of view for 20s succeeds"), T.IsSucceeded());
        TestTrue(TEXT("a clean tail never raises suspicion"), FMath::IsNearlyEqual(T.Suspicion(), 0.0, Eps));
    }

    // --- Drift too far -> lost. ---
    {
        FTailObjective T;
        FTailObjective::FParams TP;
        T.Configure(TP);
        for (int32 i = 0; i < 5; ++i)
        {
            const FTailResolver::FOutput C = Cook(FVector(-4000, 0, 0), O, FwdX);
            T.Update(C.Distance, C.bInView, 1.0);
        }
        TestTrue(TEXT("staying past MaxDistance beyond the grace period loses the mark"), T.IsLost());
    }

    // --- Hug the mark in view -> spotted. ---
    {
        FTailObjective T;
        FTailObjective::FParams TP;
        T.Configure(TP);
        for (int32 i = 0; i < 4; ++i)
        {
            const FTailResolver::FOutput C = Cook(FVector(300, 0, 0), O, FwdX);
            T.Update(C.Distance, C.bInView, 1.0);
        }
        TestTrue(TEXT("hugging the mark in its view fills suspicion and blows the tail"), T.IsSpotted());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
