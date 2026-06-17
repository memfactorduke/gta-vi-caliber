// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../LootTable.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_loot_table.gd. Float compares use Eps.
//
// RNG note: rolls use FRandomStream (seed-reproducible WITHIN UE5, not
// byte-identical to Godot). Statistical tests below pick seeds verified to pass
// under FRandomStream; see the per-test comments where a seed was pinned.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootDefaultNonEmptyTest,
    "GTC.Systems.PaySprayLoot.LootTable.DefaultTableNonEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootDefaultNonEmptyTest::RunTest(const FString& Parameters)
{
    LootTable T;
    TestTrue(TEXT("entry_count >= 4"), T.EntryCount() >= 4);
    TestTrue(TEXT("total_weight > 0"), T.TotalWeight() > 0.0f);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootDefaultTotalWeightTest,
    "GTC.Systems.PaySprayLoot.LootTable.DefaultTableTotalWeight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootDefaultTotalWeightTest::RunTest(const FString& Parameters)
{
    LootTable T;
    TestEqual(TEXT("total_weight == 15"), (double)T.TotalWeight(), 15.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootCustomTableUsedTest,
    "GTC.Systems.PaySprayLoot.LootTable.CustomTableUsed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootCustomTableUsedTest::RunTest(const FString& Parameters)
{
    LootTable T({ FLootEntry(TEXT("gold"), 2.0f, 1, 1) });
    TestEqual(TEXT("entry_count == 1"), T.EntryCount(), 1);
    TestEqual(TEXT("total_weight == 2"), (double)T.TotalWeight(), 2.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootRollValidEntryTest,
    "GTC.Systems.PaySprayLoot.LootTable.RollReturnsValidEntry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootRollValidEntryTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream Rng(12345);
    TSet<FString> Ids{ TEXT("cash"), TEXT("pistol_ammo"), TEXT("smg_ammo"), TEXT("body_armor"), TEXT("") };
    const FLootDrop Drop = T.Roll(Rng);
    TestTrue(TEXT("id is valid"), Ids.Contains(Drop.Id));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootRollQuantityInRangeTest,
    "GTC.Systems.PaySprayLoot.LootTable.RollQuantityInRange",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootRollQuantityInRangeTest::RunTest(const FString& Parameters)
{
    LootTable T({ FLootEntry(TEXT("cash"), 1.0f, 50, 200) });
    FRandomStream Rng(777);
    for (int32 I = 0; I < 50; ++I)
    {
        const FLootDrop Drop = T.Roll(Rng);
        TestTrue(TEXT("quantity in [50, 200]"), Drop.Quantity >= 50 && Drop.Quantity <= 200);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootFixedSeedReproducesTest,
    "GTC.Systems.PaySprayLoot.LootTable.FixedSeedReproducesSequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootFixedSeedReproducesTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream RngA(2024);
    FRandomStream RngB(2024);
    const TArray<FLootDrop> A = T.RollMany(RngA, 20);
    const TArray<FLootDrop> B = T.RollMany(RngB, 20);
    TestEqual(TEXT("sizes match"), A.Num(), B.Num());
    for (int32 I = 0; I < A.Num(); ++I)
    {
        TestEqual(TEXT("id matches"), A[I].Id, B[I].Id);
        TestEqual(TEXT("quantity matches"), A[I].Quantity, B[I].Quantity);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootMakeRngReproducesTest,
    "GTC.Systems.PaySprayLoot.LootTable.MakeRngReproduces",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootMakeRngReproducesTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream RngA = LootTable::MakeRng(99);
    FRandomStream RngB = LootTable::MakeRng(99);
    const FLootDrop A = T.Roll(RngA);
    const FLootDrop B = T.Roll(RngB);
    TestEqual(TEXT("id matches"), A.Id, B.Id);
    TestEqual(TEXT("quantity matches"), A.Quantity, B.Quantity);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootDifferentSeedDiffersTest,
    "GTC.Systems.PaySprayLoot.LootTable.DifferentSeedCanDiffer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootDifferentSeedDiffersTest::RunTest(const FString& Parameters)
{
    // Statistical test (8 rolls). Oracle uses seeds 1 and 2; verified to diverge
    // under FRandomStream, so the original seed constants are kept (test-local).
    LootTable T;
    FRandomStream RngA(1);
    FRandomStream RngB(2);
    const TArray<FLootDrop> A = T.RollMany(RngA, 8);
    const TArray<FLootDrop> B = T.RollMany(RngB, 8);
    bool bSame = true;
    for (int32 I = 0; I < A.Num(); ++I)
    {
        if (A[I].Id != B[I].Id || A[I].Quantity != B[I].Quantity)
        {
            bSame = false;
        }
    }
    TestFalse(TEXT("sequences differ"), bSame);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootRollManyReturnsNTest,
    "GTC.Systems.PaySprayLoot.LootTable.RollManyReturnsN",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootRollManyReturnsNTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream Rng(5);
    TestEqual(TEXT("roll_many(13) size 13"), T.RollMany(Rng, 13).Num(), 13);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootRollManyZeroNegTest,
    "GTC.Systems.PaySprayLoot.LootTable.RollManyZeroOrNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootRollManyZeroNegTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream RngA(5);
    FRandomStream RngB(5);
    TestEqual(TEXT("roll_many(0) empty"), T.RollMany(RngA, 0).Num(), 0);
    TestEqual(TEXT("roll_many(-4) empty"), T.RollMany(RngB, -4).Num(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootWeightingHeavyBeatsLightTest,
    "GTC.Systems.PaySprayLoot.LootTable.WeightingHeavyBeatsLight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootWeightingHeavyBeatsLightTest::RunTest(const FString& Parameters)
{
    // Statistical test (1000 rolls): heavy weight 10 vs light weight 1 — heavy
    // must dominate. Oracle seed 42 kept; verified to pass under FRandomStream
    // (heavy ~10x light, well clear of the margin).
    LootTable T({
        FLootEntry(TEXT("heavy"), 10.0f, 1, 1),
        FLootEntry(TEXT("light"), 1.0f, 1, 1),
    });
    FRandomStream Rng(42);
    int32 Heavy = 0;
    int32 Light = 0;
    for (int32 I = 0; I < 1000; ++I)
    {
        const FLootDrop Drop = T.Roll(Rng);
        if (Drop.Id == TEXT("heavy"))
        {
            ++Heavy;
        }
        else
        {
            ++Light;
        }
    }
    TestTrue(TEXT("heavy > light"), Heavy > Light);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootDropChanceOneTest,
    "GTC.Systems.PaySprayLoot.LootTable.DropChanceOneAlways",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootDropChanceOneTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream Rng(9);
    for (int32 I = 0; I < 20; ++I)
    {
        TestTrue(TEXT("chance 1.0 always"), T.DropChanceSatisfied(Rng, 1.0f));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootDropChanceZeroTest,
    "GTC.Systems.PaySprayLoot.LootTable.DropChanceZeroNever",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootDropChanceZeroTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream Rng(9);
    for (int32 I = 0; I < 20; ++I)
    {
        TestFalse(TEXT("chance 0.0 never"), T.DropChanceSatisfied(Rng, 0.0f));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootDropChanceClampedTest,
    "GTC.Systems.PaySprayLoot.LootTable.DropChanceClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootDropChanceClampedTest::RunTest(const FString& Parameters)
{
    LootTable T;
    FRandomStream Rng(9);
    TestTrue(TEXT("5.0 clamps to always"), T.DropChanceSatisfied(Rng, 5.0f));
    TestFalse(TEXT("-2.0 clamps to never"), T.DropChanceSatisfied(Rng, -2.0f));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootExpectedValueHandTest,
    "GTC.Systems.PaySprayLoot.LootTable.ExpectedValueHandComputed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootExpectedValueHandTest::RunTest(const FString& Parameters)
{
    // total = 4. EV = (3/4)*100*1.0 + (1/4)*0*0 = 75.0.
    LootTable T({
        FLootEntry(TEXT("cash"), 3.0f, 50, 150),
        FLootEntry(TEXT(""), 1.0f, 0, 0),
    });
    TMap<FString, float> ValueOf;
    ValueOf.Add(TEXT("cash"), 1.0f);
    TestEqual(TEXT("EV == 75"), (double)T.ExpectedValue(ValueOf), 75.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootExpectedValueMissingTest,
    "GTC.Systems.PaySprayLoot.LootTable.ExpectedValueMissingIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootExpectedValueMissingTest::RunTest(const FString& Parameters)
{
    LootTable T({ FLootEntry(TEXT("cash"), 1.0f, 10, 10) });
    TestEqual(TEXT("EV == 0"), (double)T.ExpectedValue(TMap<FString, float>()), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootEmptyTableRollSafeTest,
    "GTC.Systems.PaySprayLoot.LootTable.EmptyTableRollSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootEmptyTableRollSafeTest::RunTest(const FString& Parameters)
{
    // Empty input falls back to default; force a truly empty table instead
    // (Entries is public, mirroring the oracle's `t.entries = []`).
    LootTable T{ TArray<FLootEntry>() };
    T.Entries.Empty();
    FRandomStream Rng(1);
    const FLootDrop Drop = T.Roll(Rng);
    TestEqual(TEXT("empty id"), Drop.Id, FString());
    TestEqual(TEXT("quantity 0"), Drop.Quantity, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootZeroWeightRollSafeTest,
    "GTC.Systems.PaySprayLoot.LootTable.ZeroWeightTableRollSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootZeroWeightRollSafeTest::RunTest(const FString& Parameters)
{
    LootTable T({ FLootEntry(TEXT("cash"), 0.0f, 1, 9) });
    FRandomStream Rng(1);
    const FLootDrop Drop = T.Roll(Rng);
    TestEqual(TEXT("empty id"), Drop.Id, FString());
    TestEqual(TEXT("quantity 0"), Drop.Quantity, 0);
    TestEqual(TEXT("total_weight 0"), (double)T.TotalWeight(), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootZeroWeightEVZeroTest,
    "GTC.Systems.PaySprayLoot.LootTable.ZeroWeightExpectedValueZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootZeroWeightEVZeroTest::RunTest(const FString& Parameters)
{
    LootTable T({ FLootEntry(TEXT("cash"), 0.0f, 1, 9) });
    TMap<FString, float> ValueOf;
    ValueOf.Add(TEXT("cash"), 5.0f);
    TestEqual(TEXT("EV == 0"), (double)T.ExpectedValue(ValueOf), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootSwappedMinMaxTest,
    "GTC.Systems.PaySprayLoot.LootTable.SwappedMinMaxHandled",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootSwappedMinMaxTest::RunTest(const FString& Parameters)
{
    // min > max gets swapped during normalise, so quantity stays in [5, 10].
    LootTable T({ FLootEntry(TEXT("cash"), 1.0f, 10, 5) });
    FRandomStream Rng(321);
    for (int32 I = 0; I < 50; ++I)
    {
        const FLootDrop Drop = T.Roll(Rng);
        TestTrue(TEXT("quantity in [5, 10]"), Drop.Quantity >= 5 && Drop.Quantity <= 10);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FLootEmptyDropQuantityZeroTest,
    "GTC.Systems.PaySprayLoot.LootTable.EmptyDropQuantityZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FLootEmptyDropQuantityZeroTest::RunTest(const FString& Parameters)
{
    LootTable T({ FLootEntry(TEXT(""), 1.0f, 5, 5) });
    FRandomStream Rng(1);
    const FLootDrop Drop = T.Roll(Rng);
    TestEqual(TEXT("empty id"), Drop.Id, FString());
    TestEqual(TEXT("quantity 0"), Drop.Quantity, 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
