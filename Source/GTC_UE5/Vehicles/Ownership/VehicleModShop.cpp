// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleModShop.h"

VehicleModShop::VehicleModShop(const TArray<FCategoryDef>& Catalogue)
{
    const TArray<FCategoryDef>& Source = Catalogue.Num() > 0 ? Catalogue : DefaultCatalogue();
    for (const FCategoryDef& Def : Source)
    {
        Register(Def);
    }
}

TArray<VehicleModShop::FCategoryDef> VehicleModShop::DefaultCatalogue()
{
    return {
        FCategoryDef(TEXT("engine"), {2000, 5000, 12000}),
        FCategoryDef(TEXT("brakes"), {1500, 3500, 8000}),
        FCategoryDef(TEXT("armor"), {2500, 6000, 14000}),
        FCategoryDef(TEXT("tires"), {1000, 2500, 6000}),
    };
}

TArray<FString> VehicleModShop::Categories() const
{
    TArray<FString> Out;
    Out.Reserve(Entries.Num());
    for (const FEntry& Entry : Entries)
    {
        Out.Add(Entry.Category);
    }
    return Out;
}

bool VehicleModShop::HasCategory(const FString& Category) const
{
    return Index.Contains(Category);
}

int32 VehicleModShop::LevelOf(const FString& Category) const
{
    const FEntry* Entry = Find(Category);
    return Entry ? Entry->Level : 0;
}

int32 VehicleModShop::MaxLevel(const FString& Category) const
{
    const FEntry* Entry = Find(Category);
    return Entry ? Entry->PriceTiers.Num() : 0;
}

int32 VehicleModShop::PriceFor(const FString& Category, int32 NextLevel) const
{
    const FEntry* Entry = Find(Category);
    if (!Entry)
    {
        return -1;
    }
    if (NextLevel < 1 || NextLevel > Entry->PriceTiers.Num())
    {
        return -1;
    }
    return Entry->PriceTiers[NextLevel - 1];
}

bool VehicleModShop::CanUpgrade(const FString& Category) const
{
    const FEntry* Entry = Find(Category);
    if (!Entry)
    {
        return false;
    }
    return Entry->Level < Entry->PriceTiers.Num();
}

VehicleModShop::FModUpgradeResult VehicleModShop::Upgrade(const FString& Category, int32 Balance)
{
    FEntry* Entry = Find(Category);
    if (!Entry)
    {
        FModUpgradeResult Result;
        Result.NewBalance = Balance;
        Result.Reason = FString::Printf(TEXT("unknown category: %s"), *Category);
        return Result;
    }
    const int32 Current = Entry->Level;
    if (Current >= Entry->PriceTiers.Num())
    {
        FModUpgradeResult Result;
        Result.NewBalance = Balance;
        Result.Reason = FString::Printf(TEXT("already maxed: %s"), *Category);
        return Result;
    }
    const int32 Cost = Entry->PriceTiers[Current];
    if (Balance < Cost)
    {
        FModUpgradeResult Result;
        Result.NewBalance = Balance;
        Result.Reason = FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance);
        return Result;
    }

    Entry->Level = Current + 1;

    FModUpgradeResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    Result.NewLevel = Current + 1;
    return Result;
}

double VehicleModShop::TopSpeedMultiplier() const
{
    return 1.0 + EngineSpeedStep * static_cast<double>(LevelOf(TEXT("engine")));
}

double VehicleModShop::AccelerationMultiplier() const
{
    return 1.0 + EngineAccelStep * static_cast<double>(LevelOf(TEXT("engine")));
}

double VehicleModShop::BrakeMultiplier() const
{
    return 1.0 + BrakesStep * static_cast<double>(LevelOf(TEXT("brakes")));
}

double VehicleModShop::ArmorMultiplier() const
{
    return 1.0 + ArmorStep * static_cast<double>(LevelOf(TEXT("armor")));
}

double VehicleModShop::GripMultiplier() const
{
    return 1.0 + TiresGripStep * static_cast<double>(LevelOf(TEXT("tires")));
}

int32 VehicleModShop::TotalSpent() const
{
    int32 Spent = 0;
    for (const FEntry& Entry : Entries)
    {
        for (int32 I = 0; I < Entry.Level; ++I)
        {
            Spent += Entry.PriceTiers[I];
        }
    }
    return Spent;
}

bool VehicleModShop::IsStock() const
{
    for (const FEntry& Entry : Entries)
    {
        if (Entry.Level > 0)
        {
            return false;
        }
    }
    return true;
}

TArray<TPair<FString, int32>> VehicleModShop::Serialize() const
{
    TArray<TPair<FString, int32>> Out;
    Out.Reserve(Entries.Num());
    for (const FEntry& Entry : Entries)
    {
        Out.Emplace(Entry.Category, Entry.Level);
    }
    return Out;
}

void VehicleModShop::Restore(const TArray<TPair<FString, int32>>& State)
{
    for (const TPair<FString, int32>& Pair : State)
    {
        FEntry* Entry = Find(Pair.Key);
        if (!Entry)
        {
            continue;
        }
        Entry->Level = FMath::Clamp(Pair.Value, 0, Entry->PriceTiers.Num());
    }
}

void VehicleModShop::Reset()
{
    for (FEntry& Entry : Entries)
    {
        Entry.Level = 0;
    }
}

void VehicleModShop::Register(const FCategoryDef& Def)
{
    const FString Category = Def.Category;
    if (Category.IsEmpty() || Index.Contains(Category))
    {
        return;
    }
    TArray<int32> Clean;
    if (!CleanTiers(Def.PriceTiers, Clean))
    {
        return;
    }
    FEntry Entry;
    Entry.Category = Category;
    Entry.PriceTiers = Clean;
    Entry.Level = 0;
    const int32 NewIndex = Entries.Add(Entry);
    Index.Add(Category, NewIndex);
}

bool VehicleModShop::CleanTiers(const TArray<int32>& Tiers, TArray<int32>& OutClean)
{
    OutClean.Reset();
    if (Tiers.Num() == 0)
    {
        return false;
    }
    for (int32 Price : Tiers)
    {
        // A zero-cost tier would hand out a free permanent upgrade: reject the whole row.
        if (Price <= 0)
        {
            OutClean.Reset();
            return false;
        }
        OutClean.Add(Price);
    }
    return true;
}

const VehicleModShop::FEntry* VehicleModShop::Find(const FString& Category) const
{
    const int32* Idx = Index.Find(Category);
    return Idx ? &Entries[*Idx] : nullptr;
}

VehicleModShop::FEntry* VehicleModShop::Find(const FString& Category)
{
    const int32* Idx = Index.Find(Category);
    return Idx ? &Entries[*Idx] : nullptr;
}
