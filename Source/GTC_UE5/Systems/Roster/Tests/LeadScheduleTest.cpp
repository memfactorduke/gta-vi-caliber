// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../LeadSchedule.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;
using GtcTest::VecNear;

/**
 * Tests for FLeadSchedule — a dormant lead's daily routine. GTC-original (no Godot
 * oracle). Pins the 24h block ring (active block = last one started; before the
 * first, the overnight last block), the overnight wrap, hour folding for raw inputs,
 * input sorting + duplicate-start dedup, place/activity lookup, the
 * TimeUntilNextBlock countdown across midnight, the single-block day, and the empty
 * routine. Prefix GTC.Systems.Roster.Schedule.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace LeadScheduleTest
{
    using FBlock = FLeadSchedule::FBlock;

    // A block at StartHour, placed at (X,0,0) so positions are easy to assert.
    FBlock B(double StartHour, double X, const TCHAR* Activity)
    {
        FBlock Block;
        Block.StartHour = StartHour;
        Block.Place = FVector(X, 0.0, 0.0);
        Block.Activity = Activity;
        return Block;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLeadScheduleTest,
    "GTC.Systems.Roster.Schedule",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FLeadScheduleTest::RunTest(const FString& Parameters)
{
    using namespace LeadScheduleTest;

    // A believable day: dawn at the pier (06:00), midday downtown (12:00), nights at the bar (22:00).
    auto MakeDay = []()
    {
        TArray<FBlock> Day;
        Day.Add(B(6.0, 100.0, TEXT("fishing the pier")));
        Day.Add(B(12.0, 200.0, TEXT("downtown")));
        Day.Add(B(22.0, 300.0, TEXT("closing the bar")));
        return Day;
    };

    // ---- Empty routine is neutral everywhere ----------------------------------
    {
        FLeadSchedule S;
        TestTrue(TEXT("fresh schedule is empty"), S.IsEmpty());
        TestEqual(TEXT("no blocks"), S.BlockCount(), 0);
        TestEqual(TEXT("empty BlockAt is none"), S.BlockAt(10.0), INDEX_NONE);
        TestTrue(TEXT("empty PlaceAt is origin"), VecNear(S.PlaceAt(10.0), FVector::ZeroVector));
        TestTrue(TEXT("empty ActivityAt is blank"), S.ActivityAt(10.0).IsEmpty());
        TestTrue(TEXT("empty countdown is 0"), FMath::Abs(S.TimeUntilNextBlock(10.0)) <= Eps);
    }

    // ---- The active block is the last one already started ----------------------
    {
        FLeadSchedule S;
        S.SetRoutine(MakeDay());
        TestEqual(TEXT("three blocks"), S.BlockCount(), 3);

        TestEqual(TEXT("08:00 -> pier block"), S.BlockAt(8.0), 0);
        TestTrue(TEXT("08:00 at the pier"), VecNear(S.PlaceAt(8.0), FVector(100, 0, 0)));
        TestEqual(TEXT("08:00 activity"), S.ActivityAt(8.0), FString(TEXT("fishing the pier")));

        TestEqual(TEXT("12:00 exactly -> downtown block"), S.BlockAt(12.0), 1);
        TestEqual(TEXT("06:00 exactly -> pier block"), S.BlockAt(6.0), 0);
        TestEqual(TEXT("23:00 -> bar block"), S.BlockAt(23.0), 2);
    }

    // ---- Overnight wrap: before the first block, you're in the last block ------
    {
        FLeadSchedule S;
        S.SetRoutine(MakeDay());
        TestEqual(TEXT("02:00 is still last night's bar block"), S.BlockAt(2.0), 2);
        TestTrue(TEXT("02:00 at the bar"), VecNear(S.PlaceAt(2.0), FVector(300, 0, 0)));
    }

    // ---- Raw hours are folded modulo 24 ---------------------------------------
    {
        FLeadSchedule S;
        S.SetRoutine(MakeDay());
        TestEqual(TEXT("26:00 folds to 02:00 -> bar"), S.BlockAt(26.0), 2);
        TestEqual(TEXT("-1:00 folds to 23:00 -> bar"), S.BlockAt(-1.0), 2);
        TestEqual(TEXT("38:00 folds to 14:00 -> downtown"), S.BlockAt(38.0), 1);
    }

    // ---- Unsorted input is sorted; the active block tracks the clock, not order -
    {
        FLeadSchedule S;
        TArray<FBlock> Shuffled;
        Shuffled.Add(B(22.0, 300.0, TEXT("bar")));
        Shuffled.Add(B(6.0, 100.0, TEXT("pier")));
        Shuffled.Add(B(12.0, 200.0, TEXT("downtown")));
        S.SetRoutine(MoveTemp(Shuffled));
        TestEqual(TEXT("sorted: 08:00 -> pier is index 0"), S.BlockAt(8.0), 0);
        TestTrue(TEXT("sorted: 08:00 at the pier"), VecNear(S.PlaceAt(8.0), FVector(100, 0, 0)));
        TestTrue(TEXT("sorted: 23:00 at the bar"), VecNear(S.PlaceAt(23.0), FVector(300, 0, 0)));
    }

    // ---- A duplicate start hour keeps the first block given --------------------
    {
        FLeadSchedule S;
        TArray<FBlock> Dupes;
        Dupes.Add(B(9.0, 111.0, TEXT("first")));
        Dupes.Add(B(9.0, 999.0, TEXT("second")));
        S.SetRoutine(MoveTemp(Dupes));
        TestEqual(TEXT("duplicate start deduped"), S.BlockCount(), 1);
        TestEqual(TEXT("kept the first activity"), S.ActivityAt(10.0), FString(TEXT("first")));
    }

    // ---- TimeUntilNextBlock counts down, wrapping past midnight ----------------
    {
        FLeadSchedule S;
        S.SetRoutine(MakeDay());
        TestTrue(TEXT("08:00 -> 4h to noon"), FMath::IsNearlyEqual(S.TimeUntilNextBlock(8.0), 4.0, Eps));
        TestTrue(TEXT("23:00 -> 7h to 06:00"), FMath::IsNearlyEqual(S.TimeUntilNextBlock(23.0), 7.0, Eps));
        TestTrue(TEXT("02:00 -> 4h to 06:00"), FMath::IsNearlyEqual(S.TimeUntilNextBlock(2.0), 4.0, Eps));
    }

    // ---- A single-block day never changes block; countdown is to next day ------
    {
        FLeadSchedule S;
        TArray<FBlock> One;
        One.Add(B(9.0, 500.0, TEXT("all day")));
        S.SetRoutine(MoveTemp(One));
        TestEqual(TEXT("single block active at 03:00"), S.BlockAt(3.0), 0);
        TestEqual(TEXT("single block active at 20:00"), S.BlockAt(20.0), 0);
        TestTrue(TEXT("10:00 -> 23h until the block restarts"),
            FMath::IsNearlyEqual(S.TimeUntilNextBlock(10.0), 23.0, Eps));
        TestTrue(TEXT("08:00 -> 1h until the block starts"),
            FMath::IsNearlyEqual(S.TimeUntilNextBlock(8.0), 1.0, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
