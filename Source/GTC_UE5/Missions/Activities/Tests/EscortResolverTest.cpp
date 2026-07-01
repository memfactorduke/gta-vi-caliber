// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../EscortResolver.h"
#include "../EscortObjective.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FEscortResolver — the escortee route progress (monotonic high-water) and
 * threat-from-hostiles cook the live UGTCEscortObjectiveComponent feeds to FEscortObjective,
 * plus a few full escorts end-to-end. Prefix GTC.Missions.Activities.EscortResolver.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FEscortResolverTest,
    "GTC.Missions.Activities.EscortResolver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FEscortResolver::FOutput Cook(const FVector& Escortee, const FVector& Dest, double Route,
    double Prev, const TArray<FVector>& Hostiles)
{
    FEscortResolver::FParams P;
    FEscortResolver::FInput In;
    In.EscorteePos = Escortee;
    In.Destination = Dest;
    In.RouteLength = Route;
    In.PrevProgress = Prev;
    In.HostilePositions = Hostiles;
    return FEscortResolver::Cook(In, P);
}

bool FEscortResolverTest::RunTest(const FString& Parameters)
{
    const FVector O(0, 0, 0);
    const TArray<FVector> None;

    // --- Route progress. ---
    TestTrue(TEXT("at the start -> 0 progress"), FMath::IsNearlyEqual(Cook(FVector(1000, 0, 0), O, 1000, 0, None).Progress, 0.0, Eps));
    TestTrue(TEXT("halfway of the reachable span -> 0.5"), FMath::IsNearlyEqual(Cook(FVector(750, 0, 0), O, 1000, 0, None).Progress, 0.5, Eps));
    TestTrue(TEXT("within the arrival radius -> delivered progress"), Cook(FVector(400, 0, 0), O, 1000, 0, None).Progress == 1.0);
    TestTrue(TEXT("backtracking awards no progress"), FMath::IsNearlyEqual(Cook(FVector(1000, 0, 0), O, 1000, 0.6, None).ProgressDelta, 0.0, Eps));

    // --- Threat cooking. ---
    TestTrue(TEXT("no hostiles -> no threat"), Cook(O, O, 1000, 0, None).ThreatLevel == 0.0);
    TestTrue(TEXT("one point-blank attacker -> 1/saturation"), FMath::IsNearlyEqual(Cook(O, O, 1000, 0, TArray<FVector>{ O }).ThreatLevel, 1.0 / 3.0, Eps));
    TestTrue(TEXT("three point-blank attackers -> full threat"), FMath::IsNearlyEqual(Cook(O, O, 1000, 0, TArray<FVector>{ O, O, O }).ThreatLevel, 1.0, Eps));
    TestTrue(TEXT("an attacker at the threat radius -> no threat"), Cook(O, O, 1000, 0, TArray<FVector>{ FVector(3000, 0, 0) }).ThreatLevel == 0.0);

    // --- End-to-end: clean escort delivers with full integrity. ---
    {
        FEscortObjective E;
        FEscortObjective::FParams EP;
        E.Configure(EP);
        double High = 0.0;
        for (int32 i = 1; i <= 20; ++i)
        {
            const FVector Pos(FMath::Max(0.0, 10000.0 - i * 600.0), 0, 0);
            const FEscortResolver::FOutput C = Cook(Pos, O, 10000, High, None);
            High = C.Progress;
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        TestTrue(TEXT("a clean escort to the drop delivers"), E.IsDelivered());
        TestTrue(TEXT("an unthreatened escortee stays at full integrity"), FMath::IsNearlyEqual(E.Integrity(), 1.0, Eps));
    }

    // --- End-to-end: sustained fire loses the escortee; suppression recovers it. ---
    {
        FEscortObjective E;
        FEscortObjective::FParams EP;
        E.Configure(EP);
        const TArray<FVector> Swarm{ O, O, O };
        for (int32 i = 0; i < 6; ++i)
        {
            const FEscortResolver::FOutput C = Cook(O, FVector(10000, 0, 0), 10000, 0, Swarm);
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        TestTrue(TEXT("sustained full threat loses the escortee"), E.IsLost());
    }
    {
        FEscortObjective E;
        FEscortObjective::FParams EP;
        E.Configure(EP);
        const TArray<FVector> Swarm{ O, O, O };
        for (int32 i = 0; i < 2; ++i)
        {
            const FEscortResolver::FOutput C = Cook(O, FVector(10000, 0, 0), 10000, 0, Swarm);
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        const double Hurt = E.Integrity();
        for (int32 i = 0; i < 3; ++i)
        {
            const FEscortResolver::FOutput C = Cook(O, FVector(10000, 0, 0), 10000, 0, None);
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        TestTrue(TEXT("with the threat suppressed the escortee recovers"), E.Integrity() > Hurt);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
