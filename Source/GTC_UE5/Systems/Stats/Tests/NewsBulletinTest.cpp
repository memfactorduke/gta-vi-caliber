// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NewsBulletin.h"
#include "../StatTracker.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to a func in the Godot parity oracle
// game/tests/unit/test_news_bulletin.gd (13 funcs), with identical literals. Headline
// text is exact; counts are exact. Compound `a and b` oracle returns are split into
// separate assertions. test_count_from_stat_tracker is a real composition assertion
// (a crime count from FStatTracker fills a {count} headline) with independent constants.

// test_kinds_present
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsKindsPresentTest,
    "GTC.Systems.News.KindsPresent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsKindsPresentTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    TestTrue(TEXT("has crime"), N.Kinds().Contains(TEXT("crime")));
    TestTrue(TEXT("has heist"), N.Kinds().Contains(TEXT("heist")));
    return true;
}

// test_fresh_is_empty
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsFreshIsEmptyTest,
    "GTC.Systems.News.FreshIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsFreshIsEmptyTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    TestFalse(TEXT("no pending"), N.HasPending());
    TestEqual(TEXT("pending_count == 0"), N.PendingCount(), 0);
    TestEqual(TEXT("peek_latest empty"), N.PeekLatest(), FString());
    return true;
}

// test_report_enqueues_headline
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsReportEnqueuesTest,
    "GTC.Systems.News.ReportEnqueuesHeadline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsReportEnqueuesTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    const FString Text = N.Report(TEXT("crime"), 1, TEXT("South Beach"));
    TestEqual(TEXT("headline text"), Text, FString(TEXT("Petty crime reported in South Beach.")));
    TestEqual(TEXT("pending_count == 1"), N.PendingCount(), 1);
    return true;
}

// test_severity_picks_wording
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsSeverityPicksWordingTest,
    "GTC.Systems.News.SeverityPicksWording",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsSeverityPicksWordingTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    const FString Low = N.Report(TEXT("crime"), 1, TEXT("downtown"));
    const FString High = N.Report(TEXT("crime"), 5, TEXT("downtown"));
    TestNotEqual(TEXT("low != high"), Low, High);
    TestEqual(TEXT("high text"), High, FString(TEXT("City-wide manhunt underway across downtown.")));
    return true;
}

// test_severity_clamps_to_top_template
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsSeverityClampsTest,
    "GTC.Systems.News.SeverityClampsToTopTemplate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsSeverityClampsTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    // rampage has 3 tiers; severity 5 clamps to the last.
    const FString Text = N.Report(TEXT("rampage"), 5, TEXT("the docks"));
    TestEqual(TEXT("clamped text"), Text, FString(TEXT("the docks locked down amid a violent rampage.")));
    return true;
}

// test_place_defaults
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsPlaceDefaultsTest,
    "GTC.Systems.News.PlaceDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsPlaceDefaultsTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    // Default place "the city" (Godot context.get("place", "the city")).
    TestEqual(TEXT("default place"), N.Report(TEXT("crime"), 1), FString(TEXT("Petty crime reported in the city.")));
    return true;
}

// test_unknown_kind_is_ignored
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsUnknownKindIgnoredTest,
    "GTC.Systems.News.UnknownKindIsIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsUnknownKindIgnoredTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    TestEqual(TEXT("unknown returns empty"), N.Report(TEXT("gossip"), 1), FString());
    TestFalse(TEXT("nothing pending"), N.HasPending());
    return true;
}

// test_next_bulletin_is_fifo
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsNextBulletinFifoTest,
    "GTC.Systems.News.NextBulletinIsFifo",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsNextBulletinFifoTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    N.Report(TEXT("crime"), 1, TEXT("A"));
    N.Report(TEXT("heist"), 1, TEXT("B"));
    const FString First = N.NextBulletin();
    const FString Second = N.NextBulletin();
    TestTrue(TEXT("first contains A"), First.Contains(TEXT("A")));
    TestTrue(TEXT("second contains B"), Second.Contains(TEXT("B")));
    TestFalse(TEXT("nothing pending"), N.HasPending());
    return true;
}

// test_next_bulletin_filler_when_empty
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsFillerWhenEmptyTest,
    "GTC.Systems.News.NextBulletinFillerWhenEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsFillerWhenEmptyTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    TestEqual(TEXT("filler"), N.NextBulletin(), FNewsBulletin::Filler);
    return true;
}

// test_queue_bounded
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsQueueBoundedTest,
    "GTC.Systems.News.QueueBounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsQueueBoundedTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    for (int32 i = 0; i < FNewsBulletin::MaxQueue + 5; ++i)
    {
        N.Report(TEXT("crime"), 1);
    }
    TestEqual(TEXT("count == MaxQueue"), N.PendingCount(), FNewsBulletin::MaxQueue);
    return true;
}

// test_recent_newest_first
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsRecentNewestFirstTest,
    "GTC.Systems.News.RecentNewestFirst",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsRecentNewestFirstTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    N.Report(TEXT("crime"), 1, TEXT("first"));
    N.Report(TEXT("crime"), 1, TEXT("second"));
    const TArray<FString> Recent = N.Recent(2);
    TestEqual(TEXT("recent size 2"), Recent.Num(), 2);
    TestTrue(TEXT("recent[0] second"), Recent[0].Contains(TEXT("second")));
    TestTrue(TEXT("recent[1] first"), Recent[1].Contains(TEXT("first")));
    return true;
}

// test_clear_empties_queue
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsClearEmptiesTest,
    "GTC.Systems.News.ClearEmptiesQueue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsClearEmptiesTest::RunTest(const FString& Parameters)
{
    FNewsBulletin N;
    N.Report(TEXT("heist"), 3);
    N.Clear();
    TestFalse(TEXT("nothing pending"), N.HasPending());
    return true;
}

// test_count_from_stat_tracker — composition: a crime count from FStatTracker fills a
// {count} headline (parity with the oracle, independent constants).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNewsCountFromStatTrackerTest,
    "GTC.Systems.News.CountFromStatTracker",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FNewsCountFromStatTrackerTest::RunTest(const FString& Parameters)
{
    FStatTracker ST;
    ST.Add(TEXT("crimes"), 1.0);
    ST.Add(TEXT("crimes"), 1.0);
    ST.Add(TEXT("crimes"), 1.0);
    FNewsBulletin N;
    const FString Text = N.Report(TEXT("spree"), 2, TEXT("the city"), static_cast<int32>(ST.GetStat(TEXT("crimes"))));
    TestEqual(TEXT("count headline"), Text, FString(TEXT("Crime spree: 3 offences and counting.")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
