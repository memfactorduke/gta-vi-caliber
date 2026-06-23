// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RaceSession.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

namespace GtcRaceSessionTest
{
    constexpr double RsR = 5.0; // gate radius, matching StreetRace's R

    // A two-gate sprint: clear gate 0 then gate 1 to finish a 1-lap race.
    static TArray<FVector> Sprint()
    {
        return TArray<FVector>{
            FVector(0.0, 0.0, 0.0),
            FVector(100.0, 0.0, 0.0),
        };
    }

    // Drive a single participant all the way to the finish (assumes Racing has started).
    static void DriveToFinish(FRaceSession& Session, int32 Index, int32 Laps)
    {
        const TArray<FVector> Gates = Sprint();
        for (int32 Lap = 0; Lap < Laps; ++Lap)
        {
            for (const FVector& G : Gates)
            {
                Session.Reached(Index, G, RsR);
            }
        }
    }
}

// --- construction / phase --------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionStartsReadyWithCountdownTest,
    "GTC.Missions.Activities.RaceSession.StartsReadyWithCountdown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionStartsReadyWithCountdownTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(3, GtcRaceSessionTest::Sprint(), 1, 3.0);
    TestEqual(TEXT("participants == 3"), Session.NumParticipants(), 3);
    TestTrue(TEXT("ready"), Session.IsReady());
    TestFalse(TEXT("not racing"), Session.IsRacing());
    TestFalse(TEXT("not finished"), Session.IsFinished());
    TestEqual(TEXT("countdown == 3"), Session.CountdownRemaining(), 3.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionZeroCountdownStartsRacingTest,
    "GTC.Missions.Activities.RaceSession.ZeroCountdownStartsRacing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionZeroCountdownStartsRacingTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    TestTrue(TEXT("racing"), Session.IsRacing());
    TestEqual(TEXT("countdown == 0"), Session.CountdownRemaining(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionNegativeCountdownFlooredTest,
    "GTC.Missions.Activities.RaceSession.NegativeCountdownFlooredToZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionNegativeCountdownFlooredTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, -5.0);
    TestTrue(TEXT("racing"), Session.IsRacing());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionEmptyFieldFinishedTest,
    "GTC.Missions.Activities.RaceSession.EmptyFieldStartsFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionEmptyFieldFinishedTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(0, GtcRaceSessionTest::Sprint(), 1, 3.0);
    TestEqual(TEXT("participants == 0"), Session.NumParticipants(), 0);
    TestTrue(TEXT("finished"), Session.IsFinished());
    TestEqual(TEXT("no results"), Session.Results().Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionNegativeParticipantsFlooredTest,
    "GTC.Missions.Activities.RaceSession.NegativeParticipantsFlooredToEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionNegativeParticipantsFlooredTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(-4, GtcRaceSessionTest::Sprint(), 1, 3.0);
    TestEqual(TEXT("participants == 0"), Session.NumParticipants(), 0);
    TestTrue(TEXT("finished"), Session.IsFinished());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionDegenerateTrackFinishedTest,
    "GTC.Missions.Activities.RaceSession.DegenerateTrackStartsFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionDegenerateTrackFinishedTest::RunTest(const FString& Parameters)
{
    // No checkpoints -> every StreetRace begins finished -> the whole field is settled.
    FRaceSession Session(3, TArray<FVector>{}, 1, 3.0);
    TestTrue(TEXT("finished"), Session.IsFinished());
    TestEqual(TEXT("countdown == 0"), Session.CountdownRemaining(), 0.0, Eps);
    // All three still appear in the (empty-track) tally as DNF non-finishers... actually
    // empty-checkpoint StreetRace reports IsFinished()==true, so they tally as finishers.
    const TArray<FRaceSession::FResult> R = Session.Results();
    TestEqual(TEXT("three results"), R.Num(), 3);
    return true;
}

// --- countdown / tick ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionCountdownBurnsDownTest,
    "GTC.Missions.Activities.RaceSession.CountdownBurnsDownThenRaces",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionCountdownBurnsDownTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 3.0);
    Session.Tick(1.0);
    TestTrue(TEXT("still ready"), Session.IsReady());
    TestEqual(TEXT("countdown == 2"), Session.CountdownRemaining(), 2.0, Eps);
    Session.Tick(2.0); // exactly hits zero
    TestTrue(TEXT("racing"), Session.IsRacing());
    TestEqual(TEXT("countdown == 0"), Session.CountdownRemaining(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionGatesRefusedDuringCountdownTest,
    "GTC.Missions.Activities.RaceSession.GatesRefusedDuringCountdown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionGatesRefusedDuringCountdownTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 3.0);
    // No jumping the start: a gate crossing on the grid is a no-op.
    const bool bHit = Session.Reached(0, FVector(0.0, 0.0, 0.0), GtcRaceSessionTest::RsR);
    TestFalse(TEXT("refused"), bHit);
    TestEqual(TEXT("still at gate 0"), Session.Participant(0).CheckpointIndex(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionTickRollsOvershootIntoRaceTest,
    "GTC.Missions.Activities.RaceSession.TickRollsCountdownOvershootIntoRaceClock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionTickRollsOvershootIntoRaceTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 3.0);
    // One coarse 5s tick: 3s burns the countdown, the extra 2s becomes the first race slice.
    Session.Tick(5.0);
    TestTrue(TEXT("racing"), Session.IsRacing());
    TestEqual(TEXT("p0 elapsed == 2"), Session.Participant(0).Elapsed(), 2.0, Eps);
    TestEqual(TEXT("p1 elapsed == 2"), Session.Participant(1).Elapsed(), 2.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionTickNoopWhenFinishedTest,
    "GTC.Missions.Activities.RaceSession.TickNoopWhenFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionTickNoopWhenFinishedTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(1, GtcRaceSessionTest::Sprint(), 1, 0.0);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1);
    TestTrue(TEXT("finished"), Session.IsFinished());
    const double Before = Session.Participant(0).Elapsed();
    Session.Tick(10.0); // ignored
    TestEqual(TEXT("elapsed unchanged"), Session.Participant(0).Elapsed(), Before, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionNegativeTickNoopTest,
    "GTC.Missions.Activities.RaceSession.NegativeTickNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionNegativeTickNoopTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 3.0);
    Session.Tick(-1.0);
    TestTrue(TEXT("still ready"), Session.IsReady());
    TestEqual(TEXT("countdown unchanged"), Session.CountdownRemaining(), 3.0, Eps);
    return true;
}

// --- racing / finish -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionFinishesWhenAllInTest,
    "GTC.Missions.Activities.RaceSession.FinishesWhenAllParticipantsIn",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionFinishesWhenAllInTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1);
    TestTrue(TEXT("still racing - p1 out"), Session.IsRacing()); // p1 hasn't finished
    GtcRaceSessionTest::DriveToFinish(Session, 1, 1);
    TestTrue(TEXT("finished"), Session.IsFinished());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionReachedRefusedAfterFinishTest,
    "GTC.Missions.Activities.RaceSession.ReachedRefusedAfterParticipantFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionReachedRefusedAfterFinishTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1);
    // p0 is already in; another gate crossing for p0 is a no-op (still running == false).
    const bool bHit = Session.Reached(0, FVector(0.0, 0.0, 0.0), GtcRaceSessionTest::RsR);
    TestFalse(TEXT("refused"), bHit);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionReachedInvalidIndexTest,
    "GTC.Missions.Activities.RaceSession.ReachedInvalidIndexIsNoop",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionReachedInvalidIndexTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    TestFalse(TEXT("neg index"), Session.Reached(-1, FVector::ZeroVector, GtcRaceSessionTest::RsR));
    TestFalse(TEXT("over index"), Session.Reached(9, FVector::ZeroVector, GtcRaceSessionTest::RsR));
    return true;
}

// --- retirement (DNF) ------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionRetireRanksLastTest,
    "GTC.Missions.Activities.RaceSession.RetiredRanksBelowNonFinishers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionRetireRanksLastTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    // p1 retires while p0 is still on the grid with zero progress.
    TestTrue(TEXT("retire ok"), Session.Retire(1));
    const TArray<FRaceSession::FResult> R = Session.Results();
    // p0 (running, 0 progress) ranks ahead of p1 (DNF), even though both made no progress.
    TestEqual(TEXT("first is p0"), R[0].ParticipantIndex, 0);
    TestFalse(TEXT("p0 not finished"), R[0].bFinished);
    TestEqual(TEXT("last is p1"), R[1].ParticipantIndex, 1);
    TestEqual(TEXT("p1 reward 0"), R[1].Reward, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionRetireLastFinishesTest,
    "GTC.Missions.Activities.RaceSession.RetiringLastRunnerFinishesRace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionRetireLastFinishesTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1); // p0 done, p1 still out
    TestTrue(TEXT("still racing"), Session.IsRacing());
    TestTrue(TEXT("retire p1"), Session.Retire(1));
    TestTrue(TEXT("finished"), Session.IsFinished());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionRetireIdempotentTest,
    "GTC.Missions.Activities.RaceSession.RetireIsOneShotAndGuarded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionRetireIdempotentTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    TestTrue(TEXT("first retire"), Session.Retire(0));
    TestFalse(TEXT("second retire noop"), Session.Retire(0)); // already retired -> not running
    TestFalse(TEXT("invalid index"), Session.Retire(7));
    return true;
}

// --- ranking / results -----------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionFinisherBeatsRunnerTest,
    "GTC.Missions.Activities.RaceSession.FinisherRanksAboveStillRunning",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionFinisherBeatsRunnerTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    // p1 finishes; p0 only clears one gate.
    GtcRaceSessionTest::DriveToFinish(Session, 1, 1);
    Session.Reached(0, FVector(0.0, 0.0, 0.0), GtcRaceSessionTest::RsR);
    const TArray<FRaceSession::FResult> R = Session.Results();
    TestEqual(TEXT("first is p1"), R[0].ParticipantIndex, 1);
    TestTrue(TEXT("p1 finished"), R[0].bFinished);
    TestEqual(TEXT("p1 placement 1"), R[0].Placement, 1);
    TestEqual(TEXT("second is p0"), R[1].ParticipantIndex, 0);
    TestFalse(TEXT("p0 not finished"), R[1].bFinished);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionFasterFinisherWinsTest,
    "GTC.Missions.Activities.RaceSession.FasterFinisherRanksFirst",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionFasterFinisherWinsTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    // Both started racing at t=0. Advance 5s of shared clock, then p1 finishes first.
    Session.Tick(5.0);
    GtcRaceSessionTest::DriveToFinish(Session, 1, 1); // p1 finishes at 5s
    Session.Tick(3.0);                                 // only p0 still running -> +3s
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1); // p0 finishes at 8s
    TestTrue(TEXT("finished"), Session.IsFinished());
    const TArray<FRaceSession::FResult> R = Session.Results();
    TestEqual(TEXT("winner p1 (5s)"), R[0].ParticipantIndex, 1);
    TestEqual(TEXT("p1 elapsed 5"), R[0].Elapsed, 5.0, Eps);
    TestEqual(TEXT("p1 placement 1"), R[0].Placement, 1);
    TestEqual(TEXT("runner p0 (8s)"), R[1].ParticipantIndex, 0);
    TestEqual(TEXT("p0 elapsed 8"), R[1].Elapsed, 8.0, Eps);
    TestEqual(TEXT("p0 placement 2"), R[1].Placement, 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionTieKeepsLowerIndexTest,
    "GTC.Missions.Activities.RaceSession.ExactTieKeepsLowerIndexAhead",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionTieKeepsLowerIndexTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0);
    // Both finish in the same elapsed time (no Tick between them == 0s each).
    GtcRaceSessionTest::DriveToFinish(Session, 1, 1);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1);
    TestTrue(TEXT("finished"), Session.IsFinished());
    const TArray<FRaceSession::FResult> R = Session.Results();
    // Dead heat at 0s -> the player (index 0) takes the place.
    TestEqual(TEXT("first is p0"), R[0].ParticipantIndex, 0);
    TestEqual(TEXT("p0 placement 1"), R[0].Placement, 1);
    TestEqual(TEXT("second is p1"), R[1].ParticipantIndex, 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionRewardByPlacementTest,
    "GTC.Missions.Activities.RaceSession.ResultRewardScalesByPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionRewardByPlacementTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0, 1000);
    Session.Tick(1.0);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1); // p0 first at 1s
    Session.Tick(1.0);
    GtcRaceSessionTest::DriveToFinish(Session, 1, 1); // p1 second at 2s
    const TArray<FRaceSession::FResult> R = Session.Results();
    // StreetRace::Reward: 1st -> full 1000, 2nd -> 0.75 * 1000 == 750.
    TestEqual(TEXT("p0 reward 1000"), R[0].Reward, 1000);
    TestEqual(TEXT("p1 reward 750"), R[1].Reward, 750);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionDnfRewardZeroTest,
    "GTC.Missions.Activities.RaceSession.DnfResultRewardIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionDnfRewardZeroTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0, 1000);
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1); // p0 finishes
    Session.Retire(1);                                 // p1 DNF
    TestTrue(TEXT("finished"), Session.IsFinished());
    const TArray<FRaceSession::FResult> R = Session.Results();
    TestEqual(TEXT("winner p0"), R[0].ParticipantIndex, 0);
    TestEqual(TEXT("p0 reward 1000"), R[0].Reward, 1000);
    TestEqual(TEXT("dnf p1"), R[1].ParticipantIndex, 1);
    TestFalse(TEXT("p1 not finished"), R[1].bFinished);
    TestEqual(TEXT("p1 reward 0"), R[1].Reward, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionLivePlacementTest,
    "GTC.Missions.Activities.RaceSession.LivePlacementMidRace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionLivePlacementTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(3, GtcRaceSessionTest::Sprint(), 1, 0.0);
    // p1 clears a gate, p0 and p2 still at the start.
    Session.Reached(1, FVector(0.0, 0.0, 0.0), GtcRaceSessionTest::RsR);
    TestEqual(TEXT("p1 leads"), Session.PlacementOf(1), 1);
    // p0 and p2 are tied at zero progress; the lower index (p0) is 2nd, p2 is 3rd.
    TestEqual(TEXT("p0 second"), Session.PlacementOf(0), 2);
    TestEqual(TEXT("p2 third"), Session.PlacementOf(2), 3);
    TestEqual(TEXT("invalid placement 0"), Session.PlacementOf(9), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionProgressOfTest,
    "GTC.Missions.Activities.RaceSession.ProgressOfTracksParticipant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionProgressOfTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0); // 2 gates, 1 lap
    TestEqual(TEXT("start 0"), Session.ProgressOf(0), 0.0, Eps);
    Session.Reached(0, FVector(0.0, 0.0, 0.0), GtcRaceSessionTest::RsR); // 1 of 2
    TestEqual(TEXT("half"), Session.ProgressOf(0), 0.5, Eps);
    TestEqual(TEXT("invalid 0"), Session.ProgressOf(-3), 0.0, Eps);
    return true;
}

// --- reset / re-arm --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionResetReturnsToGridTest,
    "GTC.Missions.Activities.RaceSession.ResetReturnsToGrid",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionResetReturnsToGridTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 3.0);
    Session.Tick(3.0); // green
    Session.Reached(0, FVector(0.0, 0.0, 0.0), GtcRaceSessionTest::RsR);
    Session.Retire(1);
    Session.Reset();
    TestTrue(TEXT("ready again"), Session.IsReady());
    TestEqual(TEXT("countdown restored"), Session.CountdownRemaining(), 3.0, Eps);
    TestEqual(TEXT("p0 gate 0"), Session.Participant(0).CheckpointIndex(), 0);
    TestEqual(TEXT("p0 progress 0"), Session.ProgressOf(0), 0.0, Eps);
    // p1 is no longer retired: clearing the grid wipes DNF, so it is running again.
    TestTrue(TEXT("p1 can retire again"), Session.Retire(1));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionStartCountdownReArmsTest,
    "GTC.Missions.Activities.RaceSession.StartCountdownReArmsFromRacing",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionStartCountdownReArmsTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(2, GtcRaceSessionTest::Sprint(), 1, 0.0); // starts Racing
    TestTrue(TEXT("racing"), Session.IsRacing());
    Session.StartCountdown(2.0);
    TestTrue(TEXT("back to ready"), Session.IsReady());
    TestEqual(TEXT("countdown == 2"), Session.CountdownRemaining(), 2.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRaceSessionMultiLapTest,
    "GTC.Missions.Activities.RaceSession.MultiLapRaceFinishesAfterAllLaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRaceSessionMultiLapTest::RunTest(const FString& Parameters)
{
    FRaceSession Session(1, GtcRaceSessionTest::Sprint(), 2, 0.0); // 2 gates x 2 laps
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1); // only one lap cleared
    TestTrue(TEXT("still racing after lap 1"), Session.IsRacing());
    GtcRaceSessionTest::DriveToFinish(Session, 0, 1); // second lap
    TestTrue(TEXT("finished after 2 laps"), Session.IsFinished());
    return true;
}

#endif // WITH_AUTOMATION_TESTS
