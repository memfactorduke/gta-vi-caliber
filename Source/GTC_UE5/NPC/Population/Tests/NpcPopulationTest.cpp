// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcPopulation.h"
#include "../../Decision/NpcNeeds.h"
#include "../../Decision/NpcMind.h"
#include "../../Archetypes/NpcArchetypes.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FNpcPopulation — the persistent census that lets citizens live a
 * continuous off-screen life rather than being reborn fresh on every spawn.
 * Pure-core, scene-free (prefix GTC.NPC.Population).
 *
 * Covers: deterministic identity (same seed => same person), census seeding,
 * off-screen drive decay, off-screen replenishment from the chosen activity (so
 * nobody quietly starves), embodied records being left to their actor, and
 * intent continuity with FNpcMind.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcPopulationTest,
    "GTC.NPC.Population.Roster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcPopulationTest::RunTest(const FString& Parameters)
{
    // FromSeed is a pure function of the seed: same seed => identical person.
    {
        const FNpcCitizenRecord A = FNpcCitizenRecord::FromSeed(0, 1234);
        const FNpcCitizenRecord B = FNpcCitizenRecord::FromSeed(99, 1234); // StableId irrelevant
        TestEqual(TEXT("same seed -> same discipline"), A.Discipline, B.Discipline, Eps);
        TestEqual(TEXT("same seed -> same bravery"), A.Bravery, B.Bravery, Eps);
        TestEqual(TEXT("same seed -> same walk speed"), A.WalkSpeed, B.WalkSpeed, Eps);
        bool bRatesMatch = true;
        bool bNeedsMatch = true;
        for (const FString& Need : FNpcNeeds::Needs())
        {
            if (FMath::Abs(A.DecayRates[Need] - B.DecayRates[Need]) > Eps) { bRatesMatch = false; }
            if (FMath::Abs(A.Needs.Values[Need] - B.Needs.Values[Need]) > Eps) { bNeedsMatch = false; }
        }
        TestTrue(TEXT("same seed -> same decay rates"), bRatesMatch);
        TestTrue(TEXT("same seed -> same starting drives"), bNeedsMatch);
        TestEqual(TEXT("StableId stored verbatim"), A.StableId, 0);
        TestEqual(TEXT("seed stored verbatim"), A.Seed, 1234);
        TestEqual(TEXT("same seed -> same face"), A.HeadVariant, B.HeadVariant);
        TestTrue(TEXT("face variant in range"),
            A.HeadVariant >= 0 && A.HeadVariant < FNpcPopulation::NumHeadVariants);
        TestEqual(TEXT("same seed -> same hair"), A.HairVariant, B.HairVariant);
        TestTrue(TEXT("hair variant in range"),
            A.HairVariant >= 0 && A.HairVariant < FNpcPopulation::NumHairVariants);
        TestEqual(TEXT("same seed -> same outfit"), A.OutfitVariant, B.OutfitVariant);
        TestTrue(TEXT("outfit variant in range"),
            A.OutfitVariant >= 0 && A.OutfitVariant < FNpcPopulation::NumOutfitVariants);
    }

    // Decay rates carry the per-citizen variance off the shared base (within +/-30%).
    {
        const FNpcCitizenRecord R = FNpcCitizenRecord::FromSeed(0, 7);
        const double Base = FNpcPopulation::BaseDecayRate(TEXT("hunger"));
        const double Rate = R.DecayRates[TEXT("hunger")];
        TestTrue(TEXT("hunger decay rate within variance band"),
            Rate >= Base * 0.7 - Eps && Rate <= Base * 1.3 + Eps);
    }

    // Seed builds a census of N people, indexed by stable id, none embodied.
    {
        FNpcPopulation Pop;
        Pop.Seed(10, 1000);
        TestEqual(TEXT("census size"), Pop.Num(), 10);
        bool bWellFormed = true;
        for (int32 i = 0; i < Pop.Num(); ++i)
        {
            const FNpcCitizenRecord* Rec = Pop.Find(i);
            if (!Rec || Rec->StableId != i || Rec->Seed != 1000 + i || Rec->bEmbodied)
            {
                bWellFormed = false;
            }
        }
        TestTrue(TEXT("records are id-indexed, seeded, un-embodied"), bWellFormed);
        TestEqual(TEXT("Find out of range is null"), Pop.Find(10), static_cast<const FNpcCitizenRecord*>(nullptr));
    }

    // Off-screen Advance drains a drive whose activity does NOT serve it. At 5am
    // every routine is asleep (serves energy), so hunger has nothing topping it up.
    {
        FNpcPopulation Pop;
        Pop.Seed(8, 50);
        // Snapshot hunger before.
        TArray<double> Before;
        for (int32 i = 0; i < Pop.Num(); ++i) { Before.Add(Pop.Find(i)->Needs.Values[TEXT("hunger")]); }
        // Advance a few in-game hours in the dead of night (short enough that hunger
        // never bottoms out and trips the schedule-override, so decay clearly shows).
        for (int32 Step = 0; Step < 3; ++Step) { Pop.Advance(5.0, 1.0); }
        bool bAllHungrier = true;
        for (int32 i = 0; i < Pop.Num(); ++i)
        {
            if (Pop.Find(i)->Needs.Values[TEXT("hunger")] >= Before[i] - Eps) { bAllHungrier = false; }
        }
        TestTrue(TEXT("hunger decays off-screen while sleeping"), bAllHungrier);
    }

    // Off-screen, the drive the current activity serves is replenished — a sleeping
    // citizen's energy recovers rather than crashing to zero over a long night.
    // 5am is covered by every archetype's sleep block, so this holds for any seed.
    {
        FNpcPopulation Pop;
        Pop.Seed(1, 50);
        Pop.Roster[0].Needs.Values[TEXT("energy")] = 0.1;
        const double EnergyBefore = Pop.Find(0)->Needs.Values[TEXT("energy")];
        for (int32 Step = 0; Step < 4; ++Step) { Pop.Advance(5.0, 1.0); }
        const double EnergyAfter = Pop.Find(0)->Needs.Values[TEXT("energy")];
        TestTrue(TEXT("sleeping recovers energy off-screen"), EnergyAfter > EnergyBefore + Eps);
        TestEqual(TEXT("5am intent is sleep"), Pop.Find(0)->CurrentIntent.Activity, FString(TEXT("sleep")));
    }

    // An embodied record is left untouched by Advance — its actor owns it.
    {
        FNpcPopulation Pop;
        Pop.Seed(3, 9);
        const int32 Id = Pop.AcquireFreeRecord();
        TestEqual(TEXT("acquire returns first free id"), Id, 0);
        TestTrue(TEXT("acquired record is embodied"), Pop.Find(Id)->bEmbodied);
        const double HungerBefore = Pop.Find(Id)->Needs.Values[TEXT("hunger")];
        for (int32 Step = 0; Step < 6; ++Step) { Pop.Advance(3.0, 1.0); }
        TestEqual(TEXT("embodied record skipped by Advance"),
            Pop.Find(Id)->Needs.Values[TEXT("hunger")], HungerBefore, Eps);
    }

    // Acquire / release round-trip: release writes lived state home and re-opens the
    // record for the off-screen sim.
    {
        FNpcPopulation Pop;
        Pop.Seed(2, 11);
        const int32 Id = Pop.AcquireFreeRecord();
        FNpcNeeds Lived(1.0);
        Lived.Values[TEXT("fun")] = 0.123;
        FNpcIntent Intent;
        Intent.Activity = TEXT("eat");
        Intent.Place = TEXT("diner");
        Pop.ReleaseRecord(Id, Lived, Intent);
        TestFalse(TEXT("released record no longer embodied"), Pop.Find(Id)->bEmbodied);
        TestEqual(TEXT("released record carries lived drive"),
            Pop.Find(Id)->Needs.Values[TEXT("fun")], 0.123, Eps);
        TestEqual(TEXT("released record carries lived intent"),
            Pop.Find(Id)->CurrentIntent.Activity, FString(TEXT("eat")));
        // Default release does NOT remember a position...
        TestFalse(TEXT("release without position leaves bHasLastPosition false"),
            Pop.Find(Id)->bHasLastPosition);
        // ...but a release WITH a position stores it for continuity.
        const int32 Id2 = Pop.AcquireFreeRecord();
        const FVector Where(1234.0, 5678.0, 90.0);
        Pop.ReleaseRecord(Id2, Lived, Intent, Where, /*bStoreLastPosition=*/true);
        TestTrue(TEXT("release with position sets flag"), Pop.Find(Id2)->bHasLastPosition);
        TestEqual(TEXT("release stores last position"), Pop.Find(Id2)->LastPosition, Where);
    }

    // Acquire exhausts the census, then returns INDEX_NONE rather than over-claiming.
    {
        FNpcPopulation Pop;
        Pop.Seed(2, 0);
        TestEqual(TEXT("first acquire"), Pop.AcquireFreeRecord(), 0);
        TestEqual(TEXT("second acquire"), Pop.AcquireFreeRecord(), 1);
        TestEqual(TEXT("exhausted census returns INDEX_NONE"), Pop.AcquireFreeRecord(), (int32)INDEX_NONE);
    }

    // Intent after Advance matches what FNpcMind would independently decide for that
    // record's hour/needs/discipline — the census is a faithful driver, not a fork.
    {
        FNpcPopulation Pop;
        Pop.Seed(5, 321);
        Pop.Advance(14.0, 0.5);
        bool bAllMatch = true;
        for (int32 i = 0; i < Pop.Num(); ++i)
        {
            const FNpcCitizenRecord* Rec = Pop.Find(i);
            const FNpcArchetype A = FNpcArchetypes::Pick(Rec->Seed);
            const FNpcIntent Expected = FNpcMind::Decide(A.Schedule, 14.0, Rec->Needs, Rec->Discipline);
            if (Rec->CurrentIntent.Activity != Expected.Activity
                || Rec->CurrentIntent.Place != Expected.Place)
            {
                bAllMatch = false;
            }
        }
        TestTrue(TEXT("census intent matches FNpcMind"), bAllMatch);
    }

    // Advance with non-positive elapsed time is a no-op (guards against clock stalls).
    {
        FNpcPopulation Pop;
        Pop.Seed(1, 5);
        const double Before = Pop.Find(0)->Needs.Values[TEXT("hunger")];
        Pop.Advance(10.0, 0.0);
        TestEqual(TEXT("zero elapsed is a no-op"), Pop.Find(0)->Needs.Values[TEXT("hunger")], Before, Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
