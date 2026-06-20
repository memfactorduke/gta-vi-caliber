// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcRoutine.h"

/**
 * Behavioural tests for the pure-core individual-routine model: id lookup,
 * full-day coverage + overlap lint, and stable seed->routine assignment.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcRoutineTest,
    "GTC.NPC.Decision.NpcRoutine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FNpcRoutine MakeRoutine(const FString& Id, const TArray<FNpcScheduleBlock>& Blocks)
{
    FNpcRoutine R;
    R.Id = Id;
    R.DisplayName = Id;
    R.Blocks = Blocks;
    return R;
}

bool FNpcRoutineTest::RunTest(const FString& Parameters)
{
    // A clean, gap-free day (covers all 24h, wraps past midnight for sleep).
    const FNpcRoutine FullDay = MakeRoutine(TEXT("early_riser"), {
        {6.0, 9.0,  TEXT("goof_off"),  TEXT("gym")},
        {9.0, 17.0, TEXT("work"),      TEXT("office")},
        {17.0, 22.0, TEXT("socialize"), TEXT("bar")},
        {22.0, 6.0, TEXT("sleep"),     TEXT("home")},
    });

    // --- IsValid ---------------------------------------------------------------
    {
        TestTrue(TEXT("a full routine is valid"), FullDay.IsValid());
        FNpcRoutine Empty;
        TestFalse(TEXT("an empty routine is invalid"), Empty.IsValid());
        Empty.Id = TEXT("x");
        TestFalse(TEXT("an id with no blocks is invalid"), Empty.IsValid());
    }

    // --- Find (case-insensitive) ----------------------------------------------
    {
        const TArray<FNpcRoutine> Lib = { FullDay, MakeRoutine(TEXT("night_owl"), {{0.0, 24.0, TEXT("loiter"), TEXT("street")}}) };
        TestTrue(TEXT("finds by exact id"), FNpcRoutineLibrary::Find(Lib, TEXT("early_riser")) != nullptr);
        TestTrue(TEXT("finds case-insensitively"), FNpcRoutineLibrary::Find(Lib, TEXT("EARLY_RISER")) != nullptr);
        TestTrue(TEXT("misses an unknown id"), FNpcRoutineLibrary::Find(Lib, TEXT("nobody")) == nullptr);
    }

    // --- CoversFullDay ---------------------------------------------------------
    {
        TestTrue(TEXT("the full day covers every hour"), FNpcRoutineLibrary::CoversFullDay(FullDay));
        // A routine with a hole (nothing 12:00-13:00) is not full coverage.
        const FNpcRoutine Gappy = MakeRoutine(TEXT("gappy"), {
            {6.0, 12.0,  TEXT("work"), TEXT("office")},
            {13.0, 6.0,  TEXT("sleep"), TEXT("home")}, // 12-13 uncovered
        });
        TestFalse(TEXT("a routine with a gap is not full coverage"), FNpcRoutineLibrary::CoversFullDay(Gappy));
    }

    // --- FirstOverlap lint -----------------------------------------------------
    {
        TestEqual(TEXT("a clean routine has no overlap"),
            FNpcRoutineLibrary::FirstOverlap(FullDay), (int32)INDEX_NONE);
        const FNpcRoutine Clashing = MakeRoutine(TEXT("clash"), {
            {9.0, 17.0, TEXT("work"), TEXT("office")},
            {16.0, 18.0, TEXT("eat"), TEXT("diner")}, // overlaps the work block at 16-17
        });
        TestEqual(TEXT("overlap is reported at the shadowed block"),
            FNpcRoutineLibrary::FirstOverlap(Clashing), 1);
    }

    // --- PickIdForSeed: stable + wraps -----------------------------------------
    {
        const TArray<FNpcRoutine> Pool = {
            MakeRoutine(TEXT("a"), {{0.0, 24.0, TEXT("loiter"), TEXT("street")}}),
            MakeRoutine(TEXT("b"), {{0.0, 24.0, TEXT("loiter"), TEXT("park")}}),
            MakeRoutine(TEXT("c"), {{0.0, 24.0, TEXT("loiter"), TEXT("home")}}),
        };
        TestEqual(TEXT("same seed -> same routine"),
            FNpcRoutineLibrary::PickIdForSeed(Pool, 42), FNpcRoutineLibrary::PickIdForSeed(Pool, 42));
        TestEqual(TEXT("seed wraps by positive modulo"),
            FNpcRoutineLibrary::PickIdForSeed(Pool, 3), FNpcRoutineLibrary::PickIdForSeed(Pool, 0));
        TestEqual(TEXT("negative seed is safe"),
            FNpcRoutineLibrary::PickIdForSeed(Pool, -1), Pool[2].Id);
        TestTrue(TEXT("empty pool -> empty id"),
            FNpcRoutineLibrary::PickIdForSeed({}, 5).IsEmpty());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
