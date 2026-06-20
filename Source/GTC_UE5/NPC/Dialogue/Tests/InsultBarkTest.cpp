// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../InsultBark.h"

/**
 * Behavioural tests for FInsultBark: every spoken heat tier has lines and None is
 * silent, selection wraps deterministically, a voice with its own bank pops while
 * unknown voices fall back to the generic bank, and the peer-snipe channel always
 * has something to mutter.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FInsultBarkTest,
    "GTC.NPC.Dialogue.InsultBark",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FInsultBarkTest::RunTest(const FString& Parameters)
{
    const FString NoVoice = FString();

    // --- Every spoken heat has lines; None is silent --------------------------
    {
        TestEqual(TEXT("None has no lines"), FInsultBark::Count(NoVoice, ENpcVerbalHeat::None), 0);
        TestEqual(TEXT("None returns empty"), FInsultBark::Line(NoVoice, ENpcVerbalHeat::None, 0), FString());

        const ENpcVerbalHeat Spoken[] = {
            ENpcVerbalHeat::Mutter, ENpcVerbalHeat::Insult,
            ENpcVerbalHeat::Curse, ENpcVerbalHeat::SquareUp };
        for (ENpcVerbalHeat H : Spoken)
        {
            TestTrue(TEXT("spoken heat has lines"), FInsultBark::Count(NoVoice, H) > 0);
            TestNotEqual(TEXT("spoken heat line is non-empty"), FInsultBark::Line(NoVoice, H, 0), FString());
        }
    }

    // --- Index wraps (posmod) and is deterministic ----------------------------
    {
        const int32 N = FInsultBark::Count(NoVoice, ENpcVerbalHeat::Curse);
        TestEqual(TEXT("index wraps"),
            FInsultBark::Line(NoVoice, ENpcVerbalHeat::Curse, N),
            FInsultBark::Line(NoVoice, ENpcVerbalHeat::Curse, 0));
        TestNotEqual(TEXT("negative index safe"),
            FInsultBark::Line(NoVoice, ENpcVerbalHeat::Curse, -1), FString());
        TestEqual(TEXT("same voice+heat+index -> same line"),
            FInsultBark::Line(NoVoice, ENpcVerbalHeat::Insult, 4),
            FInsultBark::Line(NoVoice, ENpcVerbalHeat::Insult, 4));
    }

    // --- A voiced citizen pops; a heat it didn't author falls back to generic --
    {
        const FString Voice = TEXT("conspiracy");
        // It authored Insult/Curse — its bank is used (non-empty, in-character).
        TestTrue(TEXT("conspiracy authored an insult bank"),
            FInsultBark::Count(Voice, ENpcVerbalHeat::Insult) > 0);
        // It did NOT author Mutter -> falls back to the generic mutter bank.
        TestEqual(TEXT("unauthored heat falls back to generic count"),
            FInsultBark::Count(Voice, ENpcVerbalHeat::Mutter),
            FInsultBark::Count(NoVoice, ENpcVerbalHeat::Mutter));
        // An unknown voice falls back to generic for every heat.
        TestEqual(TEXT("unknown voice falls back to generic"),
            FInsultBark::Count(TEXT("nobody"), ENpcVerbalHeat::SquareUp),
            FInsultBark::Count(NoVoice, ENpcVerbalHeat::SquareUp));
    }

    // --- Peer snipe is always available ---------------------------------------
    {
        const int32 N = FInsultBark::PeerSnipeCount();
        TestTrue(TEXT("peer snipe has lines"), N > 0);
        TestNotEqual(TEXT("peer snipe never empty"), FInsultBark::PeerSnipe(0), FString());
        TestEqual(TEXT("peer snipe wraps"), FInsultBark::PeerSnipe(N), FInsultBark::PeerSnipe(0));
        TestNotEqual(TEXT("peer snipe negative index safe"), FInsultBark::PeerSnipe(-1), FString());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
