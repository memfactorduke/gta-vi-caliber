// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcPopulation.h"

#include "../Archetypes/NpcArchetypes.h"
#include "../Decision/NpcSchedule.h"
#include "Math/RandomStream.h"

double FNpcPopulation::BaseDecayRate(const FString& Need)
{
    // Tuned so a content citizen needs to eat/sleep/socialise across a day, not
    // every minute. Per-citizen +/-30% variance is layered on in FromSeed.
    if (Need == TEXT("energy"))  { return 0.045; } // ~22h awake before desperate
    if (Need == TEXT("hunger"))  { return 0.090; } // hungry a few times a day
    if (Need == TEXT("social"))  { return 0.060; }
    if (Need == TEXT("fun"))     { return 0.070; }
    if (Need == TEXT("hygiene")) { return 0.035; }
    return 0.0;
}

bool FNpcPopulation::ActivitySatisfies(const FString& Activity, FString& OutNeed, double& OutAmount)
{
    if (Activity == TEXT("sleep"))      { OutNeed = TEXT("energy");  OutAmount = 0.9; return true; }
    if (Activity == TEXT("eat"))        { OutNeed = TEXT("hunger");  OutAmount = 0.9; return true; }
    if (Activity == TEXT("socialize"))  { OutNeed = TEXT("social");  OutAmount = 0.8; return true; }
    if (Activity == TEXT("goof_off"))   { OutNeed = TEXT("fun");     OutAmount = 0.8; return true; }
    if (Activity == TEXT("loiter"))     { OutNeed = TEXT("fun");     OutAmount = 0.4; return true; }
    if (Activity == TEXT("freshen_up")) { OutNeed = TEXT("hygiene"); OutAmount = 0.9; return true; }
    return false;
}

FNpcCitizenRecord FNpcCitizenRecord::FromSeed(int32 StableId, int32 Seed)
{
    FNpcCitizenRecord R;
    R.StableId = StableId;
    R.Seed = Seed;

    const FNpcArchetype Archetype = FNpcArchetypes::Pick(Seed);
    R.Discipline = Archetype.Discipline;

    // NB: this draw order is contractual — AGTCCitizen::InitializeFromSeed consumes
    // the identical sequence from the same seed, so a roster record and a body
    // spawned from the raw seed are the same person bit-for-bit. Do not reorder.
    FRandomStream Rng(Seed);
    R.Bravery = Rng.FRandRange(0.10, 0.95);
    R.Curiosity = Rng.FRandRange(0.10, 0.90);
    R.WalkSpeed = Rng.FRandRange(130.0, 175.0);
    R.RunSpeed = Rng.FRandRange(380.0, 480.0);

    R.DecayRates.Reset();
    for (const FString& Need : FNpcNeeds::Needs())
    {
        R.DecayRates.Add(Need, FNpcPopulation::BaseDecayRate(Need) * Rng.FRandRange(0.7, 1.3));
    }

    // Start the day partway through each drive, varied per citizen, so the cast
    // doesn't all hit "hungry" on the same frame.
    R.Needs = FNpcNeeds(1.0);
    for (const FString& Need : FNpcNeeds::Needs())
    {
        R.Needs.Values[Need] = Rng.FRandRange(0.45, 1.0);
    }

    // Appended AFTER the contractual identity/drive draws above so adding these never
    // shifts the earlier sequence (existing seeds keep their traits + drives). The
    // cosmetic choices are the last things the seed decides — face, then hair.
    R.HeadVariant = Rng.RandHelper(FNpcPopulation::NumHeadVariants);
    R.HairVariant = Rng.RandHelper(FNpcPopulation::NumHairVariants);
    R.OutfitVariant = Rng.RandHelper(FNpcPopulation::NumOutfitVariants);

    return R;
}

void FNpcPopulation::Seed(int32 Count, int32 BaseSeed)
{
    Roster.Reset();
    Roster.Reserve(FMath::Max(0, Count));
    for (int32 i = 0; i < Count; ++i)
    {
        Roster.Add(FNpcCitizenRecord::FromSeed(i, BaseSeed + i));
    }
}

void FNpcPopulation::Advance(double Hour, double ElapsedHours)
{
    if (ElapsedHours <= 0.0)
    {
        return;
    }

    for (FNpcCitizenRecord& Rec : Roster)
    {
        if (Rec.bEmbodied)
        {
            continue; // its actor is simulating it for real this frame.
        }

        // 1. Drives slip with time.
        Rec.Needs.Decay(ElapsedHours, Rec.DecayRates);

        // 2. Routine + drives choose what this person is doing right now.
        const FNpcArchetype Archetype = FNpcArchetypes::Pick(Rec.Seed);
        Rec.CurrentIntent = FNpcMind::Decide(Archetype.Schedule, Hour, Rec.Needs, Rec.Discipline);

        // 3. They actually do it (off-screen, by fiat): credit the served drive,
        //    metered over the dwell so it recovers a little faster than it drains
        //    rather than snapping full. This is what keeps an unembodied citizen
        //    from quietly starving to a desperate spawn.
        FString Need;
        double Amount = 0.0;
        if (ActivitySatisfies(Rec.CurrentIntent.Activity, Need, Amount))
        {
            const double Credit = Amount * (ElapsedHours / FMath::Max(ActivityDwellHours, KINDA_SMALL_NUMBER));
            Rec.Needs.Satisfy(Need, Credit);
        }
    }
}

const FNpcCitizenRecord* FNpcPopulation::Find(int32 StableId) const
{
    return Roster.IsValidIndex(StableId) ? &Roster[StableId] : nullptr;
}

int32 FNpcPopulation::AcquireFreeRecord()
{
    for (FNpcCitizenRecord& Rec : Roster)
    {
        if (!Rec.bEmbodied)
        {
            Rec.bEmbodied = true;
            return Rec.StableId;
        }
    }
    return INDEX_NONE;
}

void FNpcPopulation::ReleaseRecord(
    int32 StableId,
    const FNpcNeeds& Needs,
    const FNpcIntent& Intent,
    const FVector& LastPos,
    bool bStoreLastPosition)
{
    if (!Roster.IsValidIndex(StableId))
    {
        return;
    }
    FNpcCitizenRecord& Rec = Roster[StableId];
    Rec.Needs = Needs;
    Rec.CurrentIntent = Intent;
    Rec.bEmbodied = false;
    if (bStoreLastPosition)
    {
        Rec.LastPosition = LastPos;
        Rec.bHasLastPosition = true;
    }
}

void FNpcPopulation::StoreAcquaintance(int32 StableId, const FNpcAcquaintance& Acq)
{
    if (!Roster.IsValidIndex(StableId))
    {
        return;
    }
    Roster[StableId].Acquaintances = Acq;
}
