// Copyright Epic Games, Inc. All Rights Reserved.

#include "LootTable.h"

LootTable::LootTable(const TArray<FLootEntry>& Table)
{
    if (Table.Num() == 0)
    {
        Entries = DefaultTable();
    }
    else
    {
        Entries = Normalise(Table);
    }
}

TArray<FLootEntry> LootTable::DefaultTable()
{
    return Normalise({
        FLootEntry(TEXT("cash"), 5.0f, 50, 200),
        FLootEntry(TEXT("pistol_ammo"), 4.0f, 6, 18),
        FLootEntry(TEXT("smg_ammo"), 2.0f, 10, 30),
        FLootEntry(TEXT("body_armor"), 1.0f, 1, 1),
        FLootEntry(TEXT(""), 3.0f, 0, 0),
    });
}

FRandomStream LootTable::MakeRng(int32 Seed)
{
    return FRandomStream(Seed);
}

float LootTable::TotalWeight() const
{
    float Sum = 0.0f;
    for (const FLootEntry& Entry : Entries)
    {
        Sum += FMath::Max(Entry.Weight, 0.0f);
    }
    return Sum;
}

int32 LootTable::EntryCount() const
{
    return Entries.Num();
}

FLootDrop LootTable::Roll(FRandomStream& Rng) const
{
    const float Total = TotalWeight();
    if (Total <= 0.0f)
    {
        return FLootDrop{ FString(), 0 };
    }
    const float Pick = Rng.GetFraction() * Total;
    float Cursor = 0.0f;
    for (const FLootEntry& Entry : Entries)
    {
        Cursor += FMath::Max(Entry.Weight, 0.0f);
        if (Pick < Cursor)
        {
            return RollEntry(Entry, Rng);
        }
    }
    // Float drift guard: fall back to the last weighted entry.
    return RollEntry(Entries.Last(), Rng);
}

TArray<FLootDrop> LootTable::RollMany(FRandomStream& Rng, int32 N) const
{
    TArray<FLootDrop> Out;
    const int32 Count = FMath::Max(N, 0);
    Out.Reserve(Count);
    for (int32 I = 0; I < Count; ++I)
    {
        Out.Add(Roll(Rng));
    }
    return Out;
}

bool LootTable::DropChanceSatisfied(FRandomStream& Rng, float Chance) const
{
    return Rng.GetFraction() < FMath::Clamp(Chance, 0.0f, 1.0f);
}

float LootTable::ExpectedValue(const TMap<FString, float>& ValueOf) const
{
    const float Total = TotalWeight();
    if (Total <= 0.0f)
    {
        return 0.0f;
    }
    float Sum = 0.0f;
    for (const FLootEntry& Entry : Entries)
    {
        const float Weight = FMath::Max(Entry.Weight, 0.0f);
        if (Weight <= 0.0f)
        {
            continue;
        }
        const float AvgQty = (static_cast<float>(Entry.Min) + static_cast<float>(Entry.Max)) * 0.5f;
        const float* Found = ValueOf.Find(Entry.Id);
        const float UnitValue = Found ? *Found : 0.0f;
        Sum += (Weight / Total) * AvgQty * UnitValue;
    }
    return Sum;
}

TArray<FLootEntry> LootTable::Normalise(const TArray<FLootEntry>& Table)
{
    TArray<FLootEntry> Out;
    Out.Reserve(Table.Num());
    for (const FLootEntry& Entry : Table)
    {
        FLootEntry Normalised;
        Normalised.Id = Entry.Id;
        Normalised.Weight = FMath::Max(Entry.Weight, 0.0f);
        int32 Lo = Entry.Min;
        int32 Hi = Entry.Max;
        if (Lo > Hi)
        {
            Swap(Lo, Hi);
        }
        Normalised.Min = Lo;
        Normalised.Max = Hi;
        Out.Add(Normalised);
    }
    return Out;
}

FLootDrop LootTable::RollEntry(const FLootEntry& Entry, FRandomStream& Rng)
{
    if (Entry.Id.IsEmpty())
    {
        return FLootDrop{ FString(), 0 };
    }
    const int32 Quantity = Rng.RandRange(Entry.Min, Entry.Max);
    return FLootDrop{ Entry.Id, Quantity };
}
