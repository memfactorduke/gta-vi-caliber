// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RaceProgressResolver.h"
#include "../RaceSession.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FRaceProgressResolver — the coordinate bridge (UE Z-up -> race XZ Y-up) and rival
 * pacing that the live UGTCStreetRaceComponent runs, plus a full race driven end-to-end through
 * the real FRaceSession. Prefix GTC.Missions.Activities.RaceProgress.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceProgressResolverTest,
    "GTC.Missions.Activities.RaceProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

// Replicate the component's per-tick rival driving.
static void DriveRivals(FRaceSession& S, TArray<double>& RivalProg, const TArray<double>& Paces,
    int32 Total, double Radius)
{
    const double PlayerProg = S.ProgressOf(0);
    for (int32 r = 0; r < Paces.Num(); ++r)
    {
        const int32 Idx = r + 1;
        const FRaceProgressResolver::FRivalTick RT = FRaceProgressResolver::RivalTick(
            RivalProg[r], S.Participant(Idx).Elapsed(), Paces[r], PlayerProg, 0.0, 0.0, Total);
        RivalProg[r] = RT.Progress;
        int32 Guard = 0;
        while (Guard++ <= Total && !S.Participant(Idx).IsFinished()
            && (Total - S.Participant(Idx).CheckpointsRemaining()) < RT.TargetCleared)
        {
            if (!S.Reached(Idx, S.Participant(Idx).CurrentCheckpoint(), Radius))
            {
                break;
            }
        }
    }
}

bool FRaceProgressResolverTest::RunTest(const FString& Parameters)
{
    // --- Coordinate bridge. ---
    {
        const FVector M = FRaceProgressResolver::ToRaceSpace(FVector(10, 20, 30));
        TestTrue(TEXT("ToRaceSpace routes UE X->X, UE Y->Z, drops height"),
            M.X == 10.0 && M.Y == 0.0 && M.Z == 20.0);
    }

    // --- Rival pacing. ---
    {
        const FRaceProgressResolver::FRivalTick RT =
            FRaceProgressResolver::RivalTick(0.0, 10.0, 20.0, 0.0, 0.0, 0.0, 4);
        TestTrue(TEXT("half its pace time -> halfway"), FMath::IsNearlyEqual(RT.Progress, 0.5, Eps));
        TestEqual(TEXT("half progress on a 4-gate course = 2 gates"), RT.TargetCleared, 2);
        const FRaceProgressResolver::FRivalTick RT2 =
            FRaceProgressResolver::RivalTick(0.8, 10.0, 20.0, 0.0, 0.0, 0.0, 4);
        TestTrue(TEXT("progress never drops below the high-water"), FMath::IsNearlyEqual(RT2.Progress, 0.8, Eps));
    }

    // --- Full race: fast player wins; rivals ranked by pace. ---
    {
        const TArray<FVector> Ue = { FVector(0, 0, 0), FVector(1000, 0, 0), FVector(1000, 1000, 0), FVector(0, 1000, 0) };
        TArray<FVector> Race;
        for (const FVector& C : Ue)
        {
            Race.Add(FRaceProgressResolver::ToRaceSpace(C));
        }
        const double Radius = 800.0;
        const int32 Total = 4;
        FRaceSession S(3, Race, 1, 3.0, 1000);
        TArray<double> Paces = { 20.0, 40.0 };
        TArray<double> RivalProg = { 0.0, 0.0 };

        TestFalse(TEXT("no gates cross during the countdown"), S.Reached(0, Race[0], Radius));
        S.Tick(1.0);
        S.Tick(1.0);
        S.Tick(1.0);
        TestTrue(TEXT("race goes green after the countdown"), S.IsRacing());

        for (int32 g = 0; g < 4; ++g)
        {
            S.Tick(1.0);
            DriveRivals(S, RivalProg, Paces, Total, Radius);
            S.Reached(0, FRaceProgressResolver::ToRaceSpace(Ue[g]), Radius);
        }
        TestTrue(TEXT("the fast player finishes all four gates"), S.Participant(0).IsFinished());

        int32 Safety = 0;
        while (!S.IsFinished() && Safety++ < 500)
        {
            S.Tick(1.0);
            DriveRivals(S, RivalProg, Paces, Total, Radius);
        }
        TestTrue(TEXT("the race finishes once every rival is in"), S.IsFinished());
        TestEqual(TEXT("the fast player wins"), S.PlacementOf(0), 1);
        TestEqual(TEXT("the faster rival places 2nd"), S.PlacementOf(1), 2);
        TestEqual(TEXT("the slower rival places 3rd"), S.PlacementOf(2), 3);

        const TArray<FRaceSession::FResult> Results = S.Results();
        TestEqual(TEXT("three ranked results"), Results.Num(), 3);
        TestEqual(TEXT("1st pays the full-scaled reward"), Results[0].Reward, StreetRace::Reward(1, 1000));
    }

    // --- Mapping matters: gate hits track UE horizontal distance, not height. ---
    {
        const TArray<FVector> Ue = { FVector(0, 0, 0), FVector(5000, 0, 0) };
        TArray<FVector> Race;
        for (const FVector& C : Ue)
        {
            Race.Add(FRaceProgressResolver::ToRaceSpace(C));
        }
        FRaceSession S(1, Race, 1, 0.0, 0);
        TestTrue(TEXT("solo zero-countdown race starts green"), S.IsRacing());
        TestFalse(TEXT("a gate 5000 away on UE-Y is not counted"),
            S.Reached(0, FRaceProgressResolver::ToRaceSpace(FVector(0, 5000, 0)), 800.0));
        TestTrue(TEXT("the same gate IS counted from directly above (height ignored)"),
            S.Reached(0, FRaceProgressResolver::ToRaceSpace(FVector(0, 0, 9999)), 800.0));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
