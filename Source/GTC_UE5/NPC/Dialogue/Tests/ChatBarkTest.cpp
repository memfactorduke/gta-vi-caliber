// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ChatBark.h"

/**
 * Unit tests for FChatBark — two-sided NPC small talk (openers + replies). Asserts
 * generic banks are non-empty/distinct/wrapping, voices flavour with generic
 * fallback for unknown voices, and selection is deterministic. Prefix
 * GTC.NPC.Dialogue.ChatBark.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChatBarkTest,
    "GTC.NPC.Dialogue.ChatBark",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FChatBarkTest::RunTest(const FString& Parameters)
{
    const FString Generic = TEXT(""); // empty voice -> generic bank

    // Generic banks: several distinct, non-empty lines for both halves of a chat.
    TestTrue(TEXT("generic openers exist"), FChatBark::OpenerCount(Generic) >= 3);
    TestTrue(TEXT("generic replies exist"), FChatBark::ReplyCount(Generic) >= 3);
    for (int32 i = 0; i < FChatBark::OpenerCount(Generic); ++i)
    {
        TestFalse(TEXT("opener non-empty"), FChatBark::Opener(Generic, i).IsEmpty());
    }
    for (int32 i = 0; i < FChatBark::ReplyCount(Generic); ++i)
    {
        TestFalse(TEXT("reply non-empty"), FChatBark::Reply(Generic, i).IsEmpty());
    }
    TestNotEqual(TEXT("openers within-bank differ"),
        FChatBark::Opener(Generic, 0), FChatBark::Opener(Generic, 1));
    TestNotEqual(TEXT("replies within-bank differ"),
        FChatBark::Reply(Generic, 0), FChatBark::Reply(Generic, 1));
    // An opener and a reply are different registers of speech.
    TestNotEqual(TEXT("opener differs from reply"),
        FChatBark::Opener(Generic, 0), FChatBark::Reply(Generic, 0));

    // Posmod wrap on both halves, incl. negative / large (production seeds overflow).
    {
        const int32 N = FChatBark::OpenerCount(Generic);
        TestEqual(TEXT("opener N wraps to 0"), FChatBark::Opener(Generic, N), FChatBark::Opener(Generic, 0));
        TestEqual(TEXT("opener -1 wraps"), FChatBark::Opener(Generic, -1), FChatBark::Opener(Generic, N - 1));
        TestFalse(TEXT("opener INT32_MIN valid"), FChatBark::Opener(Generic, MIN_int32).IsEmpty());
        TestFalse(TEXT("reply INT32_MAX valid"), FChatBark::Reply(Generic, MAX_int32).IsEmpty());
    }

    // Deterministic.
    TestEqual(TEXT("deterministic opener"), FChatBark::Opener(Generic, 2), FChatBark::Opener(Generic, 2));

    // Voice flavouring: an authored voice opens/replies differently from generic;
    // an unknown voice falls back to generic.
    TestNotEqual(TEXT("conspiracy opener is voiced"),
        FChatBark::Opener(TEXT("conspiracy"), 0), FChatBark::Opener(Generic, 0));
    TestNotEqual(TEXT("influencer reply is voiced"),
        FChatBark::Reply(TEXT("influencer"), 0), FChatBark::Reply(Generic, 0));
    TestEqual(TEXT("unknown voice opener falls back to generic"),
        FChatBark::Opener(TEXT("no_such_voice"), 0), FChatBark::Opener(Generic, 0));
    TestEqual(TEXT("unknown voice reply falls back to generic"),
        FChatBark::Reply(TEXT("no_such_voice"), 0), FChatBark::Reply(Generic, 0));

    // Voiced banks are themselves distinct/non-empty.
    TestTrue(TEXT("voiced opener bank has lines"), FChatBark::OpenerCount(TEXT("conspiracy")) >= 2);
    TestNotEqual(TEXT("voiced openers differ"),
        FChatBark::Opener(TEXT("conspiracy"), 0), FChatBark::Opener(TEXT("conspiracy"), 1));

    // --- Role assignment (IsOpener) must be ANTISYMMETRIC: the two sides, calling
    // with swapped args, always pick OPPOSITE roles, so exactly one opens. ---
    // Distinct seeds: lower seed opens, regardless of tiebreak.
    TestTrue(TEXT("lower seed opens"), FChatBark::IsOpener(5, 9, 100, 200));
    TestFalse(TEXT("higher seed replies"), FChatBark::IsOpener(9, 5, 200, 100));
    TestTrue(TEXT("lower seed opens even with higher tiebreak"), FChatBark::IsOpener(5, 9, 999, 1));
    // Equal seeds (the bug that made BOTH open): tiebreak decides, still antisymmetric.
    TestTrue(TEXT("equal seed: lower tiebreak opens"), FChatBark::IsOpener(7, 7, 10, 20));
    TestFalse(TEXT("equal seed: higher tiebreak replies"), FChatBark::IsOpener(7, 7, 20, 10));
    // Exhaustive antisymmetry over a few (seed,tiebreak) pairs: never both, never neither.
    {
        const int32 Seeds[] = { 0, 0, 3, 3, 12 };
        const uint32 Tie[]  = { 1u, 2u, 1u, 9u, 5u };
        for (int32 a = 0; a < 5; ++a)
        {
            for (int32 b = 0; b < 5; ++b)
            {
                if (a == b) { continue; } // distinct actors
                const bool AOpens = FChatBark::IsOpener(Seeds[a], Seeds[b], Tie[a], Tie[b]);
                const bool BOpens = FChatBark::IsOpener(Seeds[b], Seeds[a], Tie[b], Tie[a]);
                TestNotEqual(TEXT("exactly one of the pair opens"), AOpens, BOpens);
            }
        }
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
