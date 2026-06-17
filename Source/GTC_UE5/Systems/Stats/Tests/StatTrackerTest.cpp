// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StatTracker.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to a func in the Godot parity oracle
// game/tests/unit/test_stat_tracker.gd (21 funcs), with identical literals/tolerances.
// Counter values are double; ratio/percent assertions use Eps (is_equal_approx), and
// achievement/empty/count assertions are exact. Compound `a and b` oracle returns are
// split into separate TestTrue/TestEqual assertions.

namespace
{
    // Look up a stat id in an AllStats()/Serialize() pair list (oracle reads a Dictionary).
    bool StatTrackerHasKey(const TArray<TPair<FString, double>>& Pairs, const FString& Key)
    {
        for (const TPair<FString, double>& Pair : Pairs)
        {
            if (Pair.Key == Key)
            {
                return true;
            }
        }
        return false;
    }
}

// test_starts_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerStartsZeroTest,
    "GTC.Systems.StatTracker.StartsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerStartsZeroTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    TestEqual(TEXT("kills == 0"), S.GetStat(TEXT("kills")), 0.0, Eps);
    TestTrue(TEXT("all_stats empty"), S.AllStats().Num() == 0);
    TestEqual(TEXT("completion == 0"), S.CompletionPercent(), 0.0, Eps);
    return true;
}

// test_add_increments
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerAddIncrementsTest,
    "GTC.Systems.StatTracker.AddIncrements",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerAddIncrementsTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 3.0);
    TestEqual(TEXT("kills == 3"), S.GetStat(TEXT("kills")), 3.0, Eps);
    return true;
}

// test_add_accumulates
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerAddAccumulatesTest,
    "GTC.Systems.StatTracker.AddAccumulates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerAddAccumulatesTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 3.0);
    S.Add(TEXT("kills"), 4.0);
    TestEqual(TEXT("kills == 7"), S.GetStat(TEXT("kills")), 7.0, Eps);
    return true;
}

// test_add_default_amount_is_one
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerAddDefaultAmountTest,
    "GTC.Systems.StatTracker.AddDefaultAmountIsOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerAddDefaultAmountTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("headshots"));
    S.Add(TEXT("headshots"));
    TestEqual(TEXT("headshots == 2"), S.GetStat(TEXT("headshots")), 2.0, Eps);
    return true;
}

// test_negative_add_ignored
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerNegativeAddIgnoredTest,
    "GTC.Systems.StatTracker.NegativeAddIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerNegativeAddIgnoredTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 5.0);
    S.Add(TEXT("kills"), -2.0);
    TestEqual(TEXT("kills == 5"), S.GetStat(TEXT("kills")), 5.0, Eps);
    return true;
}

// test_unknown_stat_is_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerUnknownStatIsZeroTest,
    "GTC.Systems.StatTracker.UnknownStatIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerUnknownStatIsZeroTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    TestEqual(TEXT("never_set == 0"), S.GetStat(TEXT("never_set")), 0.0, Eps);
    return true;
}

// test_set_stat_overrides
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerSetStatOverridesTest,
    "GTC.Systems.StatTracker.SetStatOverrides",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerSetStatOverridesTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("distance_m"), 500.0);
    S.SetStat(TEXT("distance_m"), 42.0);
    TestEqual(TEXT("distance_m == 42"), S.GetStat(TEXT("distance_m")), 42.0, Eps);
    return true;
}

// test_all_stats_is_a_copy
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerAllStatsIsCopyTest,
    "GTC.Systems.StatTracker.AllStatsIsACopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerAllStatsIsCopyTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 2.0);
    // AllStats() returns a value copy; mutating it cannot touch the store.
    TArray<TPair<FString, double>> Snapshot = S.AllStats();
    for (TPair<FString, double>& Pair : Snapshot)
    {
        if (Pair.Key == TEXT("kills"))
        {
            Pair.Value = 999.0;
        }
    }
    TestEqual(TEXT("kills still 2"), S.GetStat(TEXT("kills")), 2.0, Eps);
    return true;
}

// test_headshot_ratio_correct
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerHeadshotRatioCorrectTest,
    "GTC.Systems.StatTracker.HeadshotRatioCorrect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerHeadshotRatioCorrectTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 10.0);
    S.Add(TEXT("headshots"), 4.0);
    TestEqual(TEXT("ratio == 0.4"), S.HeadshotRatio(), 0.4, Eps);
    return true;
}

// test_headshot_ratio_zero_kills_safe
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerHeadshotRatioZeroKillsTest,
    "GTC.Systems.StatTracker.HeadshotRatioZeroKillsSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerHeadshotRatioZeroKillsTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("headshots"), 3.0);
    TestEqual(TEXT("ratio == 0"), S.HeadshotRatio(), 0.0, Eps);
    return true;
}

// test_distance_km_conversion
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerDistanceKmConversionTest,
    "GTC.Systems.StatTracker.DistanceKmConversion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerDistanceKmConversionTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("distance_m"), 2500.0);
    TestEqual(TEXT("distance_km == 2.5"), S.DistanceKm(), 2.5, Eps);
    return true;
}

// test_is_achieved_flips_at_threshold
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerIsAchievedFlipsTest,
    "GTC.Systems.StatTracker.IsAchievedFlipsAtThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerIsAchievedFlipsTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 99.0);
    const bool bBefore = S.IsAchieved(TEXT("centurion"));
    S.Add(TEXT("kills"), 1.0);
    const bool bAfter = S.IsAchieved(TEXT("centurion"));
    TestFalse(TEXT("not achieved at 99"), bBefore);
    TestTrue(TEXT("achieved at 100"), bAfter);
    return true;
}

// test_is_achieved_stays_after_threshold
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerIsAchievedStaysTest,
    "GTC.Systems.StatTracker.IsAchievedStaysAfterThreshold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerIsAchievedStaysTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 250.0);
    TestTrue(TEXT("centurion achieved"), S.IsAchieved(TEXT("centurion")));
    return true;
}

// test_is_achieved_unknown_id
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerIsAchievedUnknownIdTest,
    "GTC.Systems.StatTracker.IsAchievedUnknownId",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerIsAchievedUnknownIdTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 9999.0);
    TestFalse(TEXT("unknown achievement"), S.IsAchieved(TEXT("no_such_achievement")));
    return true;
}

// test_achieved_list_grows
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerAchievedListGrowsTest,
    "GTC.Systems.StatTracker.AchievedListGrows",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerAchievedListGrowsTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    const int32 Empty = S.AchievedList().Num();
    S.Add(TEXT("kills"), 100.0);
    const int32 One = S.AchievedList().Num();
    S.Add(TEXT("headshots"), 50.0);
    const int32 Two = S.AchievedList().Num();
    TestEqual(TEXT("empty == 0"), Empty, 0);
    TestEqual(TEXT("one == 1"), One, 1);
    TestEqual(TEXT("two == 2"), Two, 2);
    return true;
}

// test_completion_percent_partial
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerCompletionPartialTest,
    "GTC.Systems.StatTracker.CompletionPercentPartial",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerCompletionPartialTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    // 5 achievements total; earn 1 -> 20%.
    S.Add(TEXT("kills"), 100.0);
    TestEqual(TEXT("completion == 20"), S.CompletionPercent(), 20.0, Eps);
    return true;
}

// test_completion_percent_full
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerCompletionFullTest,
    "GTC.Systems.StatTracker.CompletionPercentFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerCompletionFullTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 100.0);
    S.Add(TEXT("headshots"), 50.0);
    S.Add(TEXT("distance_m"), 10000.0);
    S.Add(TEXT("missions_passed"), 10.0);
    S.Add(TEXT("vehicles_jacked"), 25.0);
    TestEqual(TEXT("completion == 100"), S.CompletionPercent(), 100.0, Eps);
    return true;
}

// test_completion_percent_bounds
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerCompletionBoundsTest,
    "GTC.Systems.StatTracker.CompletionPercentBounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerCompletionBoundsTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    const double AtStart = S.CompletionPercent();
    S.Add(TEXT("kills"), 100.0);
    const double Mid = S.CompletionPercent();
    TestTrue(TEXT("at_start >= 0"), AtStart >= 0.0);
    TestTrue(TEXT("mid <= 100"), Mid <= 100.0);
    TestTrue(TEXT("mid > at_start"), Mid > AtStart);
    return true;
}

// test_serialize_restore_round_trip
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerSerializeRoundTripTest,
    "GTC.Systems.StatTracker.SerializeRestoreRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerSerializeRoundTripTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 120.0);
    S.Add(TEXT("headshots"), 55.0);
    S.Add(TEXT("distance_m"), 3300.0);
    const TArray<TPair<FString, double>> Snapshot = S.Serialize();
    FStatTracker Loaded;
    Loaded.Restore(Snapshot);
    TestEqual(TEXT("kills == 120"), Loaded.GetStat(TEXT("kills")), 120.0, Eps);
    TestEqual(TEXT("headshots == 55"), Loaded.GetStat(TEXT("headshots")), 55.0, Eps);
    TestEqual(TEXT("distance_m == 3300"), Loaded.GetStat(TEXT("distance_m")), 3300.0, Eps);
    TestTrue(TEXT("centurion achieved"), Loaded.IsAchieved(TEXT("centurion")));
    TestTrue(TEXT("sharpshooter achieved"), Loaded.IsAchieved(TEXT("sharpshooter")));
    return true;
}

// test_restore_malformed_resets — the Godot oracle passes {"stats": "not a dictionary"};
// a non-dictionary "stats" resets and returns. The C++ Restore takes a typed empty/clean
// snapshot for the same malformed case: Restore({}) resets then has nothing to load.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerRestoreMalformedTest,
    "GTC.Systems.StatTracker.RestoreMalformedResets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerRestoreMalformedTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 80.0);
    // Malformed/empty snapshot: Restore resets first (Godot restore() resets, then the
    // non-dictionary "stats" causes an early return leaving the clean slate).
    S.Restore(TArray<TPair<FString, double>>());
    TestEqual(TEXT("kills == 0"), S.GetStat(TEXT("kills")), 0.0, Eps);
    return true;
}

// test_reset_zeroes
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStatTrackerResetZeroesTest,
    "GTC.Systems.StatTracker.ResetZeroes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStatTrackerResetZeroesTest::RunTest(const FString& Parameters)
{
    FStatTracker S;
    S.Add(TEXT("kills"), 40.0);
    S.Add(TEXT("missions_passed"), 5.0);
    S.Reset();
    TestTrue(TEXT("all_stats empty"), S.AllStats().Num() == 0);
    TestEqual(TEXT("kills == 0"), S.GetStat(TEXT("kills")), 0.0, Eps);
    // (StatTrackerHasKey is exercised in the round-trip-style checks elsewhere.)
    TestFalse(TEXT("no kills key"), StatTrackerHasKey(S.AllStats(), TEXT("kills")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
