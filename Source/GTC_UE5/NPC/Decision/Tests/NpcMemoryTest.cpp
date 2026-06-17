// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcMemory.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcMemory, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_memory.gd. Decay tolerance mirrors the oracle's
 * absf(...) < 0.001; the category and recognise gates assert exactly.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcMemoryTest,
    "GTC.NPC.Decision.NpcMemory",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcMemoryTest::RunTest(const FString& Parameters)
{
    // test_decay_reduces_and_clamps
    {
        const double M = FNpcMemory::Decay(1.0, 2.0, 0.1); // 1.0 - 0.2
        TestEqual(TEXT("decay reduces"), M, 0.8, Eps);
        TestEqual(TEXT("decay clamps at zero"), FNpcMemory::Decay(0.05, 10.0, 0.1), 0.0, Eps);
    }

    // test_category_thresholds
    TestEqual(TEXT("category alarmed"), FNpcMemory::Category(0.9), FString(TEXT("alarmed")));
    TestEqual(TEXT("category uneasy"), FNpcMemory::Category(0.4), FString(TEXT("uneasy")));
    TestEqual(TEXT("category none"), FNpcMemory::Category(0.1), FString(TEXT("none")));

    // test_recognizes_when_remembered_and_near
    TestTrue(TEXT("recognizes when remembered and near"), FNpcMemory::Recognizes(0.8, 5.0, 12.0));

    // test_does_not_recognize_when_forgotten
    TestFalse(TEXT("does not recognize when forgotten"), FNpcMemory::Recognizes(0.1, 2.0, 12.0));

    // test_does_not_recognize_when_far
    TestFalse(TEXT("does not recognize when far"), FNpcMemory::Recognizes(0.9, 40.0, 12.0));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
