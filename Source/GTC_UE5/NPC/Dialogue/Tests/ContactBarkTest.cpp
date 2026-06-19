// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ContactBark.h"
#include "../../Decision/NpcContact.h"

/**
 * Unit tests for FContactBark — the lines a citizen speaks when the player is on
 * top of them. Asserts every real reaction has lines (generic + voice-flavoured),
 * Ignore has none, unknown/un-authored voices fall back to generic, posmod wrapping
 * is exact, and selection is deterministic. Prefix GTC.NPC.Dialogue.ContactBark.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FContactBarkTest,
    "GTC.NPC.Dialogue.ContactBark",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FContactBarkTest::RunTest(const FString& Parameters)
{
    using R = ENpcContactReaction;
    const FString Generic = TEXT(""); // empty voice -> generic bank

    // Every real reaction has at least a couple of distinct generic lines and never
    // returns empty for an in-range index.
    const R RealReactions[] = { R::Excuse, R::Annoyed, R::Flinch, R::Confront, R::Flee, R::Knockdown };
    for (const R Rr : RealReactions)
    {
        const int32 N = FContactBark::Count(Generic, Rr);
        TestTrue(TEXT("real reaction has generic lines"), N >= 2);
        for (int32 i = 0; i < N; ++i)
        {
            TestFalse(TEXT("in-range line is non-empty"), FContactBark::Line(Generic, Rr, i).IsEmpty());
        }
        // Within-bank distinctness: a duplicate-fill (copy-paste) regression would
        // otherwise pass every count/non-empty/wrap check silently.
        TestNotEqual(TEXT("within-bank lines differ"),
            FContactBark::Line(Generic, Rr, 0), FContactBark::Line(Generic, Rr, 1));
    }

    // The resting state says nothing, for any voice.
    TestEqual(TEXT("ignore has no lines"), FContactBark::Count(Generic, R::Ignore), 0);
    TestTrue(TEXT("ignore line is empty"), FContactBark::Line(Generic, R::Ignore, 0).IsEmpty());
    TestTrue(TEXT("ignore line is empty even for a voice"),
        FContactBark::Line(TEXT("mime"), R::Ignore, 0).IsEmpty());

    // Posmod wrap: any index is valid and folds into range. (Negatives too — seeds
    // can be large/negative once mixed with a counter.)
    {
        const int32 N = FContactBark::Count(Generic, R::Annoyed);
        TestEqual(TEXT("index N wraps to 0"),
            FContactBark::Line(Generic, R::Annoyed, N), FContactBark::Line(Generic, R::Annoyed, 0));
        TestEqual(TEXT("index -1 wraps to N-1"),
            FContactBark::Line(Generic, R::Annoyed, -1), FContactBark::Line(Generic, R::Annoyed, N - 1));
        TestEqual(TEXT("index 2N wraps to 0"),
            FContactBark::Line(Generic, R::Annoyed, 2 * N), FContactBark::Line(Generic, R::Annoyed, 0));
    }

    // Deterministic: same voice + index, same line.
    TestEqual(TEXT("deterministic selection"),
        FContactBark::Line(Generic, R::Confront, 3), FContactBark::Line(Generic, R::Confront, 3));

    // Distinct reactions generally speak differently (escalation reads as escalation).
    TestNotEqual(TEXT("excuse differs from confront"),
        FContactBark::Line(Generic, R::Excuse, 0), FContactBark::Line(Generic, R::Confront, 0));

    // --- Voice flavouring --------------------------------------------------------

    // A voice that authored a reaction snaps at you in-character — different from the
    // generic line (the influencer is filming; the conspiracy vendor is suspicious).
    TestNotEqual(TEXT("influencer confront is voiced, not generic"),
        FContactBark::Line(TEXT("influencer"), R::Confront, 0), FContactBark::Line(Generic, R::Confront, 0));
    TestNotEqual(TEXT("conspiracy flee is voiced, not generic"),
        FContactBark::Line(TEXT("conspiracy"), R::Flee, 0), FContactBark::Line(Generic, R::Flee, 0));
    // The mime authored even the light reactions (in stage directions), so its Excuse
    // diverges from the generic too.
    TestNotEqual(TEXT("mime excuse is voiced, not generic"),
        FContactBark::Line(TEXT("mime"), R::Excuse, 0), FContactBark::Line(Generic, R::Excuse, 0));
    // The remaining archetype voices are now flavoured too (spot-check a couple).
    TestNotEqual(TEXT("method_actor confront is voiced"),
        FContactBark::Line(TEXT("method_actor"), R::Confront, 0), FContactBark::Line(Generic, R::Confront, 0));
    TestNotEqual(TEXT("weather flee is voiced"),
        FContactBark::Line(TEXT("weather"), R::Flee, 0), FContactBark::Line(Generic, R::Flee, 0));
    TestNotEqual(TEXT("yogi knockdown is voiced"),
        FContactBark::Line(TEXT("yogi"), R::Knockdown, 0), FContactBark::Line(Generic, R::Knockdown, 0));

    // A voice falls back to generic for reactions it DIDN'T author (the influencer
    // never wrote an Excuse line), and an unknown voice falls back for everything.
    TestEqual(TEXT("influencer excuse falls back to generic"),
        FContactBark::Line(TEXT("influencer"), R::Excuse, 0), FContactBark::Line(Generic, R::Excuse, 0));
    TestEqual(TEXT("unknown voice falls back to generic"),
        FContactBark::Line(TEXT("no_such_voice"), R::Confront, 0), FContactBark::Line(Generic, R::Confront, 0));

    // Voiced banks are non-empty and wrap like any other.
    {
        const int32 N = FContactBark::Count(TEXT("doomsday"), R::Knockdown);
        TestTrue(TEXT("voiced bank has lines"), N >= 1);
        TestFalse(TEXT("voiced line non-empty"), FContactBark::Line(TEXT("doomsday"), R::Knockdown, 0).IsEmpty());
        TestEqual(TEXT("voiced index wraps"),
            FContactBark::Line(TEXT("doomsday"), R::Knockdown, N), FContactBark::Line(TEXT("doomsday"), R::Knockdown, 0));
    }

    // Voiced banks must also be free of accidental duplicate lines (the generic
    // within-bank check above doesn't reach them). Guard on Count>=2 so trimming a
    // bank to one line later doesn't break the test.
    struct FVoiced { const TCHAR* Voice; R Reaction; };
    const FVoiced VoicedMulti[] = {
        { TEXT("doomsday"), R::Annoyed }, { TEXT("doomsday"), R::Confront },
        { TEXT("doomsday"), R::Flee },    { TEXT("doomsday"), R::Knockdown },
        { TEXT("influencer"), R::Annoyed }, { TEXT("influencer"), R::Confront },
        { TEXT("influencer"), R::Flee },    { TEXT("influencer"), R::Knockdown },
        { TEXT("conspiracy"), R::Annoyed }, { TEXT("conspiracy"), R::Confront },
        { TEXT("conspiracy"), R::Flee },    { TEXT("conspiracy"), R::Knockdown },
        { TEXT("mime"), R::Annoyed },       { TEXT("mime"), R::Confront },
        { TEXT("food_critic"), R::Annoyed },{ TEXT("food_critic"), R::Confront },
        { TEXT("intern"), R::Annoyed },     { TEXT("intern"), R::Flee },
        { TEXT("method_actor"), R::Confront }, { TEXT("method_actor"), R::Knockdown },
        { TEXT("yogi"), R::Annoyed },          { TEXT("yogi"), R::Flee },
        { TEXT("stunt_double"), R::Confront }, { TEXT("stunt_double"), R::Knockdown },
        { TEXT("philosopher"), R::Annoyed },   { TEXT("philosopher"), R::Confront },
        { TEXT("life_coach"), R::Flee },       { TEXT("life_coach"), R::Knockdown },
        { TEXT("weather"), R::Annoyed },       { TEXT("weather"), R::Confront },
    };
    for (const FVoiced& VM : VoicedMulti)
    {
        if (FContactBark::Count(VM.Voice, VM.Reaction) >= 2)
        {
            TestNotEqual(TEXT("voiced within-bank lines differ"),
                FContactBark::Line(VM.Voice, VM.Reaction, 0), FContactBark::Line(VM.Voice, VM.Reaction, 1));
        }
    }

    // Sparse voices fall back to generic for the many reactions they don't author,
    // while still voicing the ones they do — pin both halves for the most
    // fallback-heavy voices.
    TestEqual(TEXT("food_critic flee falls back to generic"),
        FContactBark::Line(TEXT("food_critic"), R::Flee, 0), FContactBark::Line(Generic, R::Flee, 0));
    TestEqual(TEXT("intern knockdown falls back to generic"),
        FContactBark::Line(TEXT("intern"), R::Knockdown, 0), FContactBark::Line(Generic, R::Knockdown, 0));
    TestNotEqual(TEXT("food_critic confront stays voiced"),
        FContactBark::Line(TEXT("food_critic"), R::Confront, 0), FContactBark::Line(Generic, R::Confront, 0));

    // Pass-by acknowledgements: non-empty, wrapping, several of them.
    TestTrue(TEXT("passing has lines"), FContactBark::PassingCount() >= 2);
    {
        const int32 N = FContactBark::PassingCount();
        for (int32 i = 0; i < N; ++i)
        {
            TestFalse(TEXT("passing line non-empty"), FContactBark::PassingLine(i).IsEmpty());
        }
        TestEqual(TEXT("passing index N wraps to 0"), FContactBark::PassingLine(N), FContactBark::PassingLine(0));
        TestEqual(TEXT("passing index -1 wraps"), FContactBark::PassingLine(-1), FContactBark::PassingLine(N - 1));
        TestEqual(TEXT("passing index 2N wraps to 0"), FContactBark::PassingLine(2 * N), FContactBark::PassingLine(0));
    }

    // Production seeds are CitizenSeed + BarkCounter*7919 — large and, on int32
    // overflow, negative. Any such index must still fold to a valid, non-empty line.
    TestFalse(TEXT("INT32_MAX index is valid"), FContactBark::Line(Generic, R::Confront, MAX_int32).IsEmpty());
    TestFalse(TEXT("INT32_MIN index is valid"), FContactBark::Line(Generic, R::Confront, MIN_int32).IsEmpty());
    TestFalse(TEXT("INT32_MIN index is valid (voiced)"),
        FContactBark::Line(TEXT("influencer"), R::Confront, MIN_int32).IsEmpty());
    TestFalse(TEXT("large passing index is valid"), FContactBark::PassingLine(MAX_int32).IsEmpty());
    TestFalse(TEXT("negative passing index is valid"), FContactBark::PassingLine(MIN_int32).IsEmpty());

    return true;
}

#endif // WITH_AUTOMATION_TESTS
