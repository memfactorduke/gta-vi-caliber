// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcSchedule.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcSchedule, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_schedule.gd. Tolerance mirrors the oracle's absf(...)
 * < 0.001 checks (Eps = 1e-4 is tighter); wrap/cover/fallback assert exactly.
 *
 * Insertion order is observable (first covering block wins) and preserved by the
 * ordered TArray of blocks, matching the Godot Array routine.
 */

namespace
{
    // The 4-block day used by the oracle's _day fixture.
    TArray<FNpcScheduleBlock> MakeDay()
    {
        return {
            {6.0, 9.0, TEXT("commute"), TEXT("street")},
            {9.0, 17.0, TEXT("work"), TEXT("office")},
            {17.0, 22.0, TEXT("leisure"), TEXT("bar")},
            {22.0, 6.0, TEXT("sleep"), TEXT("home")},
        };
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcScheduleTest,
    "GTC.NPC.Decision.NpcSchedule",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcScheduleTest::RunTest(const FString& Parameters)
{
    const TArray<FNpcScheduleBlock> Day = MakeDay();

    // test_wrap_hour_normalises
    TestEqual(TEXT("wrap 26 -> 2"), FNpcSchedule::WrapHour(26.0), 2.0, Eps);
    TestEqual(TEXT("wrap -1 -> 23"), FNpcSchedule::WrapHour(-1.0), 23.0, Eps);

    // test_block_covers_simple_range
    {
        const FNpcScheduleBlock B(9.0, 17.0, FString(), FString());
        TestTrue(TEXT("simple covers 12"), FNpcSchedule::BlockCovers(B, 12.0));
        TestFalse(TEXT("simple excludes end 17"), FNpcSchedule::BlockCovers(B, 17.0));
        TestFalse(TEXT("simple excludes 8"), FNpcSchedule::BlockCovers(B, 8.0));
    }

    // test_block_covers_wrapping_range
    {
        const FNpcScheduleBlock B(22.0, 6.0, FString(), FString());
        TestTrue(TEXT("wrap covers 23"), FNpcSchedule::BlockCovers(B, 23.0));
        TestTrue(TEXT("wrap covers 2"), FNpcSchedule::BlockCovers(B, 2.0));
        TestFalse(TEXT("wrap excludes 12"), FNpcSchedule::BlockCovers(B, 12.0));
    }

    // test_activity_at_picks_right_block
    TestEqual(TEXT("activity at 10 is work"), FNpcSchedule::ActivityAt(Day, 10.0).Activity, FString(TEXT("work")));
    TestEqual(TEXT("activity at 3 is sleep"), FNpcSchedule::ActivityAt(Day, 3.0).Activity, FString(TEXT("sleep")));
    TestEqual(TEXT("activity at 19 is leisure"), FNpcSchedule::ActivityAt(Day, 19.0).Activity, FString(TEXT("leisure")));

    // test_activity_at_falls_back_to_idle
    {
        const TArray<FNpcScheduleBlock> Sparse = {{9.0, 10.0, TEXT("blink"), TEXT("void")}};
        TestEqual(TEXT("falls back to loiter"), FNpcSchedule::ActivityAt(Sparse, 15.0).Activity, FString(TEXT("loiter")));
    }

    // test_hours_until_end_simple
    {
        const FNpcScheduleBlock B(9.0, 17.0, FString(), FString());
        TestEqual(TEXT("hours until end simple"), FNpcSchedule::HoursUntilEnd(B, 16.5), 0.5, Eps);
    }

    // test_hours_until_end_wraps
    {
        const FNpcScheduleBlock B(22.0, 6.0, FString(), FString());
        // At 23:00, sleep ends at 06:00 -> 7 hours away.
        TestEqual(TEXT("hours until end wraps"), FNpcSchedule::HoursUntilEnd(B, 23.0), 7.0, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
