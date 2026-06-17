// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcArchetypes.h"
#include "../../Decision/NpcSchedule.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

/**
 * Parity tests for FNpcArchetypes, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_npc_archetypes.gd (7 funcs). Schedule coverage compares
 * the active block to FNpcSchedule::Idle() by full structural equality, matching
 * the Godot `block == NpcSchedule.IDLE` dict comparison (SLOW_LIFE reuses the
 * "loiter"/"street" pair on a non-24h window, so activity-only would be wrong).
 */

namespace
{
    // Full structural equality of a schedule block to the IDLE fallback.
    bool NpcArchetypesTest_IsIdle(const FNpcScheduleBlock& Block)
    {
        const FNpcScheduleBlock Idle = FNpcSchedule::Idle();
        return FMath::Abs(Block.Start - Idle.Start) < Eps
            && FMath::Abs(Block.End - Idle.End) < Eps
            && Block.Activity == Idle.Activity
            && Block.Place == Idle.Place;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcArchetypesTest,
    "GTC.NPC.Archetypes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcArchetypesTest::RunTest(const FString& Parameters)
{
    const TArray<FNpcArchetype>& All = FNpcArchetypes::All();

    // test_catalog_is_populated
    TestTrue(TEXT("catalog has >= 10 archetypes"), All.Num() >= 10);

    // test_every_archetype_is_well_formed
    // The Godot dict "has(key)" checks are structurally guaranteed by FNpcArchetype's
    // fields; the load-bearing runtime checks are: schedule is a non-empty array and
    // tint is a colour (FLinearColor, always true by type). Assert non-empty schedule.
    for (const FNpcArchetype& C : All)
    {
        TestTrue(TEXT("archetype schedule is non-empty"), C.Schedule.Num() > 0);
    }

    // test_ids_are_unique
    {
        TSet<FString> Seen;
        bool bUnique = true;
        for (const FNpcArchetype& C : All)
        {
            if (Seen.Contains(C.Id))
            {
                bUnique = false;
                break;
            }
            Seen.Add(C.Id);
        }
        TestTrue(TEXT("archetype ids are unique"), bUnique);
    }

    // test_schedules_cover_the_whole_clock
    // Sample every half hour; every archetype must yield a real (non-idle) block.
    {
        bool bCoversClock = true;
        for (const FNpcArchetype& C : All)
        {
            double H = 0.0;
            while (H < 24.0)
            {
                const FNpcScheduleBlock Block = FNpcSchedule::ActivityAt(C.Schedule, H);
                if (NpcArchetypesTest_IsIdle(Block))
                {
                    bCoversClock = false;
                    break;
                }
                H += 0.5;
            }
            if (!bCoversClock)
            {
                break;
            }
        }
        TestTrue(TEXT("schedules cover the whole clock with no gaps"), bCoversClock);
    }

    // test_by_id_round_trips
    {
        const FNpcArchetype& First = All[0];
        TestEqual(TEXT("by_id round-trips to same name"), FNpcArchetypes::ById(First.Id).Name, First.Name);
    }

    // test_by_id_unknown_is_empty
    TestTrue(TEXT("by_id unknown is empty"), FNpcArchetypes::ById(TEXT("nobody_real")).IsEmpty());

    // test_pick_is_deterministic_and_wraps
    {
        const int32 N = FNpcArchetypes::All().Num();
        const FNpcArchetype A = FNpcArchetypes::Pick(3);
        const FNpcArchetype B = FNpcArchetypes::Pick(3 + N); // wraps to same index
        TestEqual(TEXT("pick is deterministic and wraps"), A.Id, B.Id);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
