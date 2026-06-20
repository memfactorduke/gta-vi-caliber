// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcCrudeAction.h"

/**
 * Behavioural tests for FNpcCrudeAction. pee ("relieve_self") and vomit are disabled
 * for now, so the only crude action is the mild "spit": it can appear (rarely), the
 * disabled tokens never appear in any context, and selection is deterministic.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcCrudeActionTest,
    "GTC.NPC.Decision.NpcCrudeAction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcCrudeActionTest::RunTest(const FString& Parameters)
{
    const FName None = NAME_None;
    const uint8 Empty = 0, Quiet = 1, Busy = 2, Packed = 3;

    // --- IsNight window --------------------------------------------------------
    {
        TestTrue(TEXT("22:00 is night"), FNpcCrudeAction::IsNight(22.0));
        TestTrue(TEXT("03:00 is night"), FNpcCrudeAction::IsNight(3.0));
        TestTrue(TEXT("21:00 is night"), FNpcCrudeAction::IsNight(21.0));
        TestFalse(TEXT("noon is not night"), FNpcCrudeAction::IsNight(12.0));
        TestFalse(TEXT("05:00 is not night"), FNpcCrudeAction::IsNight(5.0));
    }

    // --- pee / vomit are disabled: they never appear, in ANY context ----------
    {
        // Sweep the contexts that used to produce them (a late bar, a quiet alley at
        // night, home, a packed crowd) across many seeds: none should ever come back.
        const TArray<FName> Places = { TEXT("bar"), TEXT("street"), TEXT("home"), TEXT("park") };
        const double Hours[] = { 2.0, 13.0, 23.0 };
        const uint8 Busys[] = { Empty, Quiet, Busy, Packed };
        bool bAnyDisabled = false;
        for (const FName& P : Places)
        {
            for (double H : Hours)
            {
                for (uint8 B : Busys)
                {
                    for (int32 Seed = 0; Seed < 200; ++Seed)
                    {
                        const FName A = FNpcCrudeAction::Pick(H, P, B, ENpcTemper::Hothead, Seed);
                        if (A == TEXT("relieve_self") || A == TEXT("vomit")) { bAnyDisabled = true; }
                    }
                }
            }
        }
        TestFalse(TEXT("pee and vomit never appear (disabled)"), bAnyDisabled);
    }

    // --- spit: the one remaining crude action still appears (rarely) -----------
    {
        int32 Spits = 0, Nones = 0;
        for (int32 Seed = 0; Seed < 1000; ++Seed)
        {
            const FName A = FNpcCrudeAction::Pick(13.0, TEXT("street"), Busy, ENpcTemper::Surly, Seed);
            if (A == TEXT("spit")) { ++Spits; }
            if (A == None) { ++Nones; }
        }
        TestTrue(TEXT("some rude sorts spit"), Spits > 0);
        TestTrue(TEXT("but most cycles are still nothing"), Nones > Spits);
    }

    // --- Determinism -----------------------------------------------------------
    {
        TestEqual(TEXT("same context+seed -> same token"),
            FNpcCrudeAction::Pick(13.0, TEXT("street"), Busy, ENpcTemper::Surly, 777),
            FNpcCrudeAction::Pick(13.0, TEXT("street"), Busy, ENpcTemper::Surly, 777));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
