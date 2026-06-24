// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcAttitude.h"

/**
 * Behavioural tests for FNpcAttitude: temper is deterministic and sanely
 * distributed (most people easygoing, a spicy minority), and a provocation maps to
 * a verbal heat that rises with temper, respects the meek cap, and gates SquareUp
 * behind being both rude and brave.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcAttitudeTest,
    "GTC.NPC.Decision.NpcAttitude",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcAttitudeTest::RunTest(const FString& Parameters)
{
    // --- Temper is deterministic in (seed, id) --------------------------------
    {
        TestEqual(TEXT("same seed+id -> same temper"),
            static_cast<int32>(FNpcAttitude::TemperFor(42, TEXT("barista"))),
            static_cast<int32>(FNpcAttitude::TemperFor(42, TEXT("barista"))));
        // Different id with the same seed can differ (id folds into the hash).
        // Not asserted as inequality (a collision is legal), just that it's stable:
        TestEqual(TEXT("id is case-insensitive"),
            static_cast<int32>(FNpcAttitude::TemperFor(7, TEXT("Vendor"))),
            static_cast<int32>(FNpcAttitude::TemperFor(7, TEXT("vendor"))));
    }

    // --- Distribution: all tempers appear; easygoing dominates, hothead rare ---
    {
        int32 Counts[4] = { 0, 0, 0, 0 };
        for (int32 Seed = 0; Seed < 2000; ++Seed)
        {
            Counts[static_cast<int32>(FNpcAttitude::TemperFor(Seed, TEXT("citizen")))]++;
        }
        TestTrue(TEXT("some meek exist"), Counts[(int32)ENpcTemper::Meek] > 0);
        TestTrue(TEXT("some hotheads exist"), Counts[(int32)ENpcTemper::Hothead] > 0);
        TestTrue(TEXT("easygoing is the plurality"),
            Counts[(int32)ENpcTemper::Easygoing] > Counts[(int32)ENpcTemper::Meek] &&
            Counts[(int32)ENpcTemper::Easygoing] > Counts[(int32)ENpcTemper::Surly] &&
            Counts[(int32)ENpcTemper::Easygoing] > Counts[(int32)ENpcTemper::Hothead]);
        TestTrue(TEXT("hotheads are rarer than the surly"),
            Counts[(int32)ENpcTemper::Hothead] < Counts[(int32)ENpcTemper::Surly]);
    }

    // --- IsRude ----------------------------------------------------------------
    {
        TestFalse(TEXT("meek not rude"), FNpcAttitude::IsRude(ENpcTemper::Meek));
        TestFalse(TEXT("easygoing not rude"), FNpcAttitude::IsRude(ENpcTemper::Easygoing));
        TestTrue(TEXT("surly is rude"), FNpcAttitude::IsRude(ENpcTemper::Surly));
        TestTrue(TEXT("hothead is rude"), FNpcAttitude::IsRude(ENpcTemper::Hothead));
    }

    // --- Heat rises with temper for the same provocation -----------------------
    // Seed=1 -> jitter slot 1 (no temper jitter), so the mapping is the clean baseline.
    {
        const int32 Seed = 1;
        const auto Bumped = ENpcProvocation::Bumped;
        const ENpcVerbalHeat Meek = FNpcAttitude::ProvocationResponse(ENpcTemper::Meek, Bumped, 0.5, Seed);
        const ENpcVerbalHeat Easy = FNpcAttitude::ProvocationResponse(ENpcTemper::Easygoing, Bumped, 0.5, Seed);
        const ENpcVerbalHeat Surl = FNpcAttitude::ProvocationResponse(ENpcTemper::Surly, Bumped, 0.5, Seed);
        TestTrue(TEXT("easygoing answers a bump hotter than the meek"),
            static_cast<int32>(Easy) > static_cast<int32>(Meek));
        TestTrue(TEXT("the surly answer hotter still"),
            static_cast<int32>(Surl) >= static_cast<int32>(Easy));
        TestTrue(TEXT("a bumped meek never exceeds Insult"),
            static_cast<int32>(Meek) <= static_cast<int32>(ENpcVerbalHeat::Insult));
    }

    // --- SquareUp is gated behind rude + brave ---------------------------------
    {
        const int32 Seed = 1;
        const ENpcVerbalHeat Brave = FNpcAttitude::ProvocationResponse(
            ENpcTemper::Hothead, ENpcProvocation::Bumped, 0.9, Seed);
        const ENpcVerbalHeat Timid = FNpcAttitude::ProvocationResponse(
            ENpcTemper::Hothead, ENpcProvocation::Bumped, 0.2, Seed);
        TestEqual(TEXT("a brave hothead squares up"), static_cast<int32>(Brave), static_cast<int32>(ENpcVerbalHeat::SquareUp));
        TestEqual(TEXT("a timid hothead only curses"), static_cast<int32>(Timid), static_cast<int32>(ENpcVerbalHeat::Curse));
    }

    // --- PeerPassing: only the rude bother, only ever a mutter -----------------
    {
        TestEqual(TEXT("easygoing ignores a passer-by"),
            static_cast<int32>(FNpcAttitude::ProvocationResponse(ENpcTemper::Easygoing, ENpcProvocation::PeerPassing, 0.5, 3)),
            static_cast<int32>(ENpcVerbalHeat::None));
        TestEqual(TEXT("the surly mutter at a passer-by"),
            static_cast<int32>(FNpcAttitude::ProvocationResponse(ENpcTemper::Surly, ENpcProvocation::PeerPassing, 0.5, 3)),
            static_cast<int32>(ENpcVerbalHeat::Mutter));
    }

    // --- A stare simmers lower than a barge ------------------------------------
    {
        const int32 Seed = 1;
        TestTrue(TEXT("being stared at draws less heat than being bumped"),
            static_cast<int32>(FNpcAttitude::ProvocationResponse(ENpcTemper::Surly, ENpcProvocation::Stared, 0.5, Seed)) <
            static_cast<int32>(FNpcAttitude::ProvocationResponse(ENpcTemper::Surly, ENpcProvocation::Bumped, 0.5, Seed)));
    }

    // --- Determinism -----------------------------------------------------------
    {
        TestEqual(TEXT("same inputs -> same response"),
            static_cast<int32>(FNpcAttitude::ProvocationResponse(ENpcTemper::Hothead, ENpcProvocation::NearMiss, 0.7, 99)),
            static_cast<int32>(FNpcAttitude::ProvocationResponse(ENpcTemper::Hothead, ENpcProvocation::NearMiss, 0.7, 99)));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
