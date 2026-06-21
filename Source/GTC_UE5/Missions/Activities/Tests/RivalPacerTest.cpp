// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../RivalPacer.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// --- BaseProgress: the honest pace clock -----------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerBaseZeroElapsedTest,
    "GTC.Missions.Activities.RivalPacer.BaseZeroElapsedIsStartLine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerBaseZeroElapsedTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("t=0 -> 0"), FRivalPacer::BaseProgress(0.0, 60.0), 0.0, Eps);
    TestEqual(TEXT("t<0 -> 0"), FRivalPacer::BaseProgress(-5.0, 60.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerBaseLinearTest,
    "GTC.Missions.Activities.RivalPacer.BaseRampsLinearlyToOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerBaseLinearTest::RunTest(const FString& Parameters)
{
    // 60s pace: quarter / half / full of the way through.
    TestEqual(TEXT("15s -> 0.25"), FRivalPacer::BaseProgress(15.0, 60.0), 0.25, Eps);
    TestEqual(TEXT("30s -> 0.5"), FRivalPacer::BaseProgress(30.0, 60.0), 0.5, Eps);
    TestEqual(TEXT("60s -> 1.0"), FRivalPacer::BaseProgress(60.0, 60.0), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerBaseClampsPastFinishTest,
    "GTC.Missions.Activities.RivalPacer.BaseClampsPastFinish",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerBaseClampsPastFinishTest::RunTest(const FString& Parameters)
{
    // Elapsed beyond the pace stays pinned at 1.0 (no overshoot).
    TestEqual(TEXT("120s of 60 -> 1.0"), FRivalPacer::BaseProgress(120.0, 60.0), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerBaseDegeneratePaceTest,
    "GTC.Missions.Activities.RivalPacer.BaseDegeneratePaceIsFinished",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerBaseDegeneratePaceTest::RunTest(const FString& Parameters)
{
    // Non-positive pace: instant rival pinned at the finish, never a div-by-zero.
    TestEqual(TEXT("pace 0 -> 1.0"), FRivalPacer::BaseProgress(10.0, 0.0), 1.0, Eps);
    TestEqual(TEXT("pace <0 -> 1.0"), FRivalPacer::BaseProgress(10.0, -3.0), 1.0, Eps);
    // ...but a degenerate pace at the start line is still the start line.
    TestEqual(TEXT("pace 0, t=0 -> 0"), FRivalPacer::BaseProgress(0.0, 0.0), 0.0, Eps);
    return true;
}

// --- RubberBand: bounded, fair nudge ---------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerRubberBandDisabledTest,
    "GTC.Missions.Activities.RivalPacer.RubberBandDisabledIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerRubberBandDisabledTest::RunTest(const FString& Parameters)
{
    // Zero strength or zero room -> no nudge at all.
    TestEqual(TEXT("strength 0"), FRivalPacer::RubberBand(0.2, 0.9, 0.0, 0.05), 0.0, Eps);
    TestEqual(TEXT("room 0"), FRivalPacer::RubberBand(0.2, 0.9, 0.3, 0.0), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerRubberBandSignTest,
    "GTC.Missions.Activities.RivalPacer.RubberBandPullsTowardPlayer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerRubberBandSignTest::RunTest(const FString& Parameters)
{
    // Player ahead -> trailing rival pulled UP (positive); player behind -> leading
    // rival shaded DOWN (negative). Gap small enough to stay under the cap.
    // strength 0.3 * (0.5 - 0.4) = +0.03
    TestEqual(TEXT("trailer up"), FRivalPacer::RubberBand(0.4, 0.5, 0.3, 0.05), 0.03, Eps);
    // strength 0.3 * (0.4 - 0.5) = -0.03
    TestEqual(TEXT("leader down"), FRivalPacer::RubberBand(0.5, 0.4, 0.3, 0.05), -0.03, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerRubberBandCapTest,
    "GTC.Missions.Activities.RivalPacer.RubberBandHardCapped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerRubberBandCapTest::RunTest(const FString& Parameters)
{
    // Player miles ahead: raw nudge 0.3 * (1.0 - 0.0) = 0.3, but capped at +0.05 so
    // a far-ahead player can never be caught for free.
    TestEqual(TEXT("up capped"), FRivalPacer::RubberBand(0.0, 1.0, 0.3, 0.05), 0.05, Eps);
    // ...and symmetrically on the way down.
    TestEqual(TEXT("down capped"), FRivalPacer::RubberBand(1.0, 0.0, 0.3, 0.05), -0.05, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerRubberBandClampsInputsTest,
    "GTC.Missions.Activities.RivalPacer.RubberBandClampsOutOfRangeInputs",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerRubberBandClampsInputsTest::RunTest(const FString& Parameters)
{
    // Out-of-range progresses are clamped before the gap is taken, so the nudge stays
    // bounded: both clamp to 1.0 -> gap 0 -> no nudge.
    TestEqual(TEXT("clamped equal"), FRivalPacer::RubberBand(1.5, 2.0, 0.3, 0.05), 0.0, Eps);
    return true;
}

// --- ProgressAt: clock + bounded nudge, clamped ----------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerProgressNoRubberBandTest,
    "GTC.Missions.Activities.RivalPacer.ProgressStrengthZeroIsBaseClock",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerProgressNoRubberBandTest::RunTest(const FString& Parameters)
{
    // Strength 0 -> exactly the honest pace clock, independent of the player.
    const double P = FRivalPacer::ProgressAt(30.0, 60.0, 0.95, 0.0, 0.05);
    TestEqual(TEXT("== base 0.5"), P, 0.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerProgressNudgesTest,
    "GTC.Missions.Activities.RivalPacer.ProgressAddsBoundedNudge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerProgressNudgesTest::RunTest(const FString& Parameters)
{
    // Base 0.5, player at 0.6: 0.3*(0.6-0.5)=+0.03 -> 0.53.
    const double P = FRivalPacer::ProgressAt(30.0, 60.0, 0.6, 0.3, 0.05);
    TestEqual(TEXT("0.5 + 0.03"), P, 0.53, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerProgressClampedTest,
    "GTC.Missions.Activities.RivalPacer.ProgressClampedToUnitRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerProgressClampedTest::RunTest(const FString& Parameters)
{
    // Base already 1.0 (finished), player far behind: the nudge is negative but the
    // cap bounds it, so a finished leader is shaded back by exactly the cap and no
    // more: 1.0 + clamp(0.3*(0-1), -0.05, 0.05) = 0.95.
    const double Top = FRivalPacer::ProgressAt(60.0, 60.0, 0.0, 0.3, 0.05);
    TestEqual(TEXT("finished leader shaded back by exactly the cap -> 0.95"), Top, 0.95, Eps);
    // A rival still at the start can't be pushed below 0 by the nudge either.
    const double Bottom = FRivalPacer::ProgressAt(0.0, 60.0, 0.0, 0.3, 0.05);
    TestEqual(TEXT("start stays 0"), Bottom, 0.0, Eps);
    return true;
}

// --- MonotonicProgressAt: never goes backwards -----------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerMonotonicHoldsTest,
    "GTC.Missions.Activities.RivalPacer.MonotonicFloorsAtPrevious",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerMonotonicHoldsTest::RunTest(const FString& Parameters)
{
    // Rival was at 0.6; player drops right back so the raw nudge would shade the
    // rival to ~0.45 — but progress reported to the adapter never reverses.
    const double Now = FRivalPacer::MonotonicProgressAt(0.6, 30.0, 60.0, 0.0, 0.3, 0.05);
    TestEqual(TEXT("held at 0.6"), Now, 0.6, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerMonotonicAdvancesTest,
    "GTC.Missions.Activities.RivalPacer.MonotonicAdvancesWhenAhead",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerMonotonicAdvancesTest::RunTest(const FString& Parameters)
{
    // The clock has moved the rival past its previous mark -> it advances.
    const double Now = FRivalPacer::MonotonicProgressAt(0.4, 45.0, 60.0, 0.75, 0.0, 0.05);
    TestEqual(TEXT("advanced to 0.75"), Now, 0.75, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerMonotonicClampsPrevTest,
    "GTC.Missions.Activities.RivalPacer.MonotonicClampsPreviousIntoRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerMonotonicClampsPrevTest::RunTest(const FString& Parameters)
{
    // A bogus >1 previous is clamped to 1.0, and the rival is treated as finished.
    const double Now = FRivalPacer::MonotonicProgressAt(2.0, 30.0, 60.0, 0.5, 0.3, 0.05);
    TestEqual(TEXT("held at 1.0"), Now, 1.0, Eps);
    return true;
}

// --- ProgressToCheckpoint: the adapter's bridge to StreetRace::Reached ------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerCheckpointMapsTest,
    "GTC.Missions.Activities.RivalPacer.CheckpointFloorsProgress",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerCheckpointMapsTest::RunTest(const FString& Parameters)
{
    // 8-gate course (4 cp x 2 laps). Half-way -> 4 cleared; a hair under the next
    // gate still floors down.
    TestEqual(TEXT("0.5 -> 4"), FRivalPacer::ProgressToCheckpoint(0.5, 8), 4);
    TestEqual(TEXT("0.62 -> 4"), FRivalPacer::ProgressToCheckpoint(0.62, 8), 4);
    TestEqual(TEXT("0.0 -> 0"), FRivalPacer::ProgressToCheckpoint(0.0, 8), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRivalPacerCheckpointClampsTest,
    "GTC.Missions.Activities.RivalPacer.CheckpointClampsEnds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRivalPacerCheckpointClampsTest::RunTest(const FString& Parameters)
{
    // Finished -> all gates; out-of-range progress clamps both ways; no gates -> 0.
    TestEqual(TEXT("1.0 -> 8"), FRivalPacer::ProgressToCheckpoint(1.0, 8), 8);
    TestEqual(TEXT(">1 -> 8"), FRivalPacer::ProgressToCheckpoint(1.5, 8), 8);
    TestEqual(TEXT("<0 -> 0"), FRivalPacer::ProgressToCheckpoint(-0.2, 8), 0);
    TestEqual(TEXT("no gates -> 0"), FRivalPacer::ProgressToCheckpoint(0.5, 0), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
