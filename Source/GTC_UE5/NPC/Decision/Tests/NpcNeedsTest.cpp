// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcNeeds.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcNeeds, mapped 1:1 from the the reference oracle
 * game/tests/unit/test_npc_needs.gd. Tolerance mirrors the oracle's absf(...)
 * < 0.001 checks (Eps = 1e-4 is tighter); exact-0/1 clamps assert exactly.
 *
 * Canonical drive order (FNpcNeeds::Needs) is the insertion-order backing store;
 * most-urgent ties resolve toward the earlier need, matching the the reference scan.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcNeedsTest,
    "GTC.NPC.Decision.NpcNeeds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcNeedsTest::RunTest(const FString& Parameters)
{
    // test_init_fills_all_needs
    {
        FNpcNeeds N(0.5);
        bool bAll = true;
        for (const FString& Need : FNpcNeeds::Needs())
        {
            if (FMath::Abs(N.Values[Need] - 0.5) > 0.001)
            {
                bAll = false;
            }
        }
        TestTrue(TEXT("init fills all needs"), bAll);
    }

    // test_decay_uses_per_need_rate
    {
        FNpcNeeds N(1.0);
        N.Decay(2.0, {{TEXT("hunger"), 0.25}});
        // hunger drained 0.25/h x 2h = 0.5; energy had no rate so it stays full.
        TestEqual(TEXT("hunger decayed"), N.Values[TEXT("hunger")], 0.5, Eps);
        TestEqual(TEXT("energy unchanged"), N.Values[TEXT("energy")], 1.0, Eps);
    }

    // test_decay_clamps_at_zero
    {
        FNpcNeeds N(0.3);
        N.Decay(10.0, {{TEXT("energy"), 1.0}});
        TestEqual(TEXT("decay clamps at zero"), N.Values[TEXT("energy")], 0.0, Eps);
    }

    // test_satisfy_clamps_at_one
    {
        FNpcNeeds N(0.9);
        N.Satisfy(TEXT("fun"), 0.5);
        TestEqual(TEXT("satisfy clamps at one"), N.Values[TEXT("fun")], 1.0, Eps);
    }

    // test_satisfy_ignores_unknown_need
    {
        FNpcNeeds N(1.0);
        N.Satisfy(TEXT("nonsense"), 0.5); // must not crash or add a key
        TestFalse(TEXT("satisfy ignores unknown need"), N.Values.Contains(TEXT("nonsense")));
    }

    // test_urgency_is_inverse_of_value
    {
        FNpcNeeds N(1.0);
        N.Values[TEXT("hunger")] = 0.2;
        TestEqual(TEXT("urgency is inverse of value"), N.Urgency(TEXT("hunger")), 0.8, Eps);
    }

    // test_most_urgent_finds_lowest
    {
        FNpcNeeds N(1.0);
        N.Values[TEXT("social")] = 0.4;
        N.Values[TEXT("hygiene")] = 0.1;
        TestEqual(TEXT("most urgent finds lowest"), N.MostUrgent(), FString(TEXT("hygiene")));
    }

    // test_most_urgent_breaks_ties_by_order
    {
        FNpcNeeds N(0.5); // everything tied
        TestEqual(TEXT("most urgent breaks ties by order"), N.MostUrgent(), FNpcNeeds::Needs()[0]);
    }

    // test_peak_urgency_matches_worst
    {
        FNpcNeeds N(1.0);
        N.Values[TEXT("energy")] = 0.15;
        TestEqual(TEXT("peak urgency matches worst"), N.PeakUrgency(), 0.85, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
