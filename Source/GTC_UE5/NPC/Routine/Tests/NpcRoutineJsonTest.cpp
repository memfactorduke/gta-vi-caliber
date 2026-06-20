// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcRoutineJson.h"
#include "../../../Tests/GtcTestTolerances.h"

/**
 * Round-trip + safety tests for the routine JSON codec: a routine survives
 * serialize->parse intact, a bank round-trips, and malformed / wrong-format text is
 * rejected safely (never a half-built routine).
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcRoutineJsonTest,
    "GTC.NPC.Routine.NpcRoutineJson",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FNpcRoutine SampleRoutine()
{
    FNpcRoutine R;
    R.Id = TEXT("barista_early_riser");
    R.DisplayName = TEXT("Early-Riser Barista");
    R.Blocks = {
        {6.0, 9.0,  TEXT("goof_off"),  TEXT("gym")},
        {9.0, 17.0, TEXT("work"),      TEXT("office")},
        {17.0, 22.0, TEXT("socialize"), TEXT("bar")},
        {22.0, 6.0, TEXT("sleep"),     TEXT("home")},
    };
    return R;
}

bool FNpcRoutineJsonTest::RunTest(const FString& Parameters)
{
    const FNpcRoutine In = SampleRoutine();

    // --- single routine round-trips -------------------------------------------
    {
        const FString Json = NpcRoutineJson::RoutineToJson(In);
        TestTrue(TEXT("json mentions the format tag"), Json.Contains(TEXT("gtc_routine")));

        FNpcRoutine Out;
        TestTrue(TEXT("parses back"), NpcRoutineJson::JsonToRoutine(Json, Out));
        TestEqual(TEXT("id preserved"), Out.Id, In.Id);
        TestEqual(TEXT("name preserved"), Out.DisplayName, In.DisplayName);
        TestEqual(TEXT("block count preserved"), Out.Blocks.Num(), In.Blocks.Num());
        for (int32 i = 0; i < In.Blocks.Num(); ++i)
        {
            TestEqual(TEXT("block start"), Out.Blocks[i].Start, In.Blocks[i].Start, GtcTest::Eps);
            TestEqual(TEXT("block end"), Out.Blocks[i].End, In.Blocks[i].End, GtcTest::Eps);
            TestEqual(TEXT("block activity"), Out.Blocks[i].Activity, In.Blocks[i].Activity);
            TestEqual(TEXT("block place"), Out.Blocks[i].Place, In.Blocks[i].Place);
        }
    }

    // --- malformed / wrong format is rejected safely --------------------------
    {
        FNpcRoutine Out;
        TestFalse(TEXT("garbage text rejected"), NpcRoutineJson::JsonToRoutine(TEXT("{not json"), Out));
        TestFalse(TEXT("rejected output is invalid"), Out.IsValid());
        TestFalse(TEXT("wrong format tag rejected"),
            NpcRoutineJson::JsonToRoutine(TEXT("{\"format\":\"gtc_mission\"}"), Out));
        TestFalse(TEXT("empty object rejected"), NpcRoutineJson::JsonToRoutine(TEXT("{}"), Out));
    }

    // --- a bank round-trips, junk entries skipped -----------------------------
    {
        FNpcRoutine R2 = In;
        R2.Id = TEXT("retiree_slow_life");
        R2.DisplayName = TEXT("Slow-Life Retiree");
        const TArray<FNpcRoutine> Bank = { In, R2 };

        const FString Json = NpcRoutineJson::BankToJson(Bank);
        TArray<FNpcRoutine> Out;
        TestTrue(TEXT("bank parses"), NpcRoutineJson::JsonToBank(Json, Out));
        TestEqual(TEXT("bank size preserved"), Out.Num(), 2);
        TestTrue(TEXT("first routine present"), FNpcRoutineLibrary::Find(Out, TEXT("barista_early_riser")) != nullptr);
        TestTrue(TEXT("second routine present"), FNpcRoutineLibrary::Find(Out, TEXT("retiree_slow_life")) != nullptr);

        // A single-routine document is not a bank.
        TArray<FNpcRoutine> NotABank;
        TestFalse(TEXT("a single-routine doc is not a bank"),
            NpcRoutineJson::JsonToBank(NpcRoutineJson::RoutineToJson(In), NotABank));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
