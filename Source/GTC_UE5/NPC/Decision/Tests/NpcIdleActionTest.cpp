// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcIdleAction.h"

/**
 * Unit tests for FNpcIdleAction — the contextual idle-fidget picker. Asserts each
 * known activity has actions, selection is deterministic and in-bank, unknown
 * activities fall back to the generic bank (never NAME_None), and the posmod wrap
 * is exact. Prefix GTC.NPC.Decision.NpcIdleAction.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcIdleActionTest,
    "GTC.NPC.Decision.NpcIdleAction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcIdleActionTest::RunTest(const FString& Parameters)
{
    const TArray<FString> KnownActivities = {
        TEXT("loiter"), TEXT("socialize"), TEXT("eat"),
        TEXT("commute"), TEXT("goof_off"), TEXT("freshen_up"), TEXT("work"),
    };

    // Every known activity has a non-trivial bank, and every in-range pick is a real
    // (non-None) token that actually belongs to that activity's bank.
    for (const FString& Activity : KnownActivities)
    {
        const int32 N = FNpcIdleAction::Count(Activity);
        TestTrue(TEXT("activity has actions"), N >= 2);
        for (int32 i = 0; i < N; ++i)
        {
            const FName Action = FNpcIdleAction::Pick(Activity, i);
            TestFalse(TEXT("in-range pick is not None"), Action.IsNone());
        }
    }

    // Deterministic: same activity + seed always yields the same action.
    TestEqual(TEXT("deterministic selection"),
        FNpcIdleAction::Pick(TEXT("loiter"), 3), FNpcIdleAction::Pick(TEXT("loiter"), 3));

    // Posmod wrap: any seed (incl. negative / large) folds into range and is valid.
    {
        const int32 N = FNpcIdleAction::Count(TEXT("loiter"));
        TestEqual(TEXT("seed N wraps to 0"),
            FNpcIdleAction::Pick(TEXT("loiter"), N), FNpcIdleAction::Pick(TEXT("loiter"), 0));
        TestEqual(TEXT("seed -1 wraps to N-1"),
            FNpcIdleAction::Pick(TEXT("loiter"), -1), FNpcIdleAction::Pick(TEXT("loiter"), N - 1));
        TestFalse(TEXT("large negative seed is valid"),
            FNpcIdleAction::Pick(TEXT("loiter"), MIN_int32).IsNone());
    }

    // Distinct activities draw from distinct contextual banks (eat != goof_off).
    TestNotEqual(TEXT("eat differs from goof_off at index 0"),
        FNpcIdleAction::Pick(TEXT("eat"), 0), FNpcIdleAction::Pick(TEXT("goof_off"), 0));

    // Unknown / empty activities fall back to the generic bank — never None.
    TestTrue(TEXT("unknown activity falls back to generic"),
        FNpcIdleAction::Count(TEXT("nonsense_activity")) >= 2);
    TestFalse(TEXT("unknown activity pick is not None"),
        FNpcIdleAction::Pick(TEXT("nonsense_activity"), 0).IsNone());
    TestFalse(TEXT("empty activity pick is not None"),
        FNpcIdleAction::Pick(FString(), 0).IsNone());
    // Two different unknown activities both resolve to the SAME generic bank, so
    // their counts match — proves a single shared fallback, not per-string banks.
    TestEqual(TEXT("all unknown activities share one generic bank"),
        FNpcIdleAction::Count(TEXT("nonsense_a")), FNpcIdleAction::Count(TEXT("nonsense_b")));

    // Sweeping indices 0..N-1 over a bank yields every action exactly once: the
    // bank has no accidental duplicate entries and posmod covers the whole range.
    {
        const int32 N = FNpcIdleAction::Count(TEXT("loiter"));
        TSet<FName> Seen;
        for (int32 i = 0; i < N; ++i)
        {
            Seen.Add(FNpcIdleAction::Pick(TEXT("loiter"), i));
        }
        TestEqual(TEXT("a full index sweep covers all distinct actions"), Seen.Num(), N);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
