// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../IdleActionBark.h"

/**
 * Unit tests for FIdleActionBark — the quiet mutters paired with idle actions.
 * Asserts talkative actions have lines, silent actions return "", posmod wrap is
 * exact, selection is deterministic. Prefix GTC.NPC.Dialogue.IdleActionBark.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FIdleActionBarkTest,
    "GTC.NPC.Dialogue.IdleActionBark",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FIdleActionBarkTest::RunTest(const FString& Parameters)
{
    // Talkative actions have non-empty banks and never return empty in range.
    const FName Talkative[] = {
        TEXT("check_phone"), TEXT("check_watch"), TEXT("people_watch"),
        TEXT("sigh"), TEXT("savor"), TEXT("sip_drink"), TEXT("laugh"), TEXT("check_reflection"),
    };
    for (const FName Action : Talkative)
    {
        const int32 N = FIdleActionBark::Count(Action);
        TestTrue(TEXT("talkative action has lines"), N >= 2);
        for (int32 i = 0; i < N; ++i)
        {
            TestFalse(TEXT("in-range line is non-empty"), FIdleActionBark::Line(Action, i).IsEmpty());
        }
    }

    // Silent actions (and unknown tokens) return "" with a zero count.
    const FName Silent[] = { TEXT("smoke"), TEXT("lean"), TEXT("stretch"), TEXT("nope_unknown") };
    for (const FName Action : Silent)
    {
        TestEqual(TEXT("silent action has no lines"), FIdleActionBark::Count(Action), 0);
        TestTrue(TEXT("silent action line is empty"), FIdleActionBark::Line(Action, 0).IsEmpty());
    }

    // Posmod wrap: any index folds into range.
    {
        const FName A = TEXT("check_phone");
        const int32 N = FIdleActionBark::Count(A);
        TestEqual(TEXT("index N wraps to 0"), FIdleActionBark::Line(A, N), FIdleActionBark::Line(A, 0));
        TestEqual(TEXT("index -1 wraps to N-1"), FIdleActionBark::Line(A, -1), FIdleActionBark::Line(A, N - 1));
        TestEqual(TEXT("index 2N wraps to 0"), FIdleActionBark::Line(A, 2 * N), FIdleActionBark::Line(A, 0));
        TestFalse(TEXT("large negative index valid"), FIdleActionBark::Line(A, MIN_int32).IsEmpty());
    }

    // Deterministic, and distinct actions read differently.
    TestEqual(TEXT("deterministic selection"),
        FIdleActionBark::Line(TEXT("check_watch"), 2), FIdleActionBark::Line(TEXT("check_watch"), 2));
    TestNotEqual(TEXT("phone differs from watch"),
        FIdleActionBark::Line(TEXT("check_phone"), 0), FIdleActionBark::Line(TEXT("check_watch"), 0));
    // The index is actually OBSERVABLE: two distinct in-range indices within one bank
    // select different lines (guards against a regression that ignored Index).
    TestNotEqual(TEXT("distinct indices select distinct lines"),
        FIdleActionBark::Line(TEXT("check_phone"), 0), FIdleActionBark::Line(TEXT("check_phone"), 1));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
