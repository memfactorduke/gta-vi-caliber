// Copyright Epic Games, Inc. All Rights Reserved.

#include "DistrictEconomy.h"

DistrictEconomy::DistrictEconomy(const TArray<FDistrictDef>& Districts)
{
    const TArray<FDistrictDef>& Source = Districts.Num() > 0 ? Districts : DefaultDistricts();
    for (const FDistrictDef& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<FDistrictDef> DistrictEconomy::DefaultDistricts()
{
    return {
        FDistrictDef(TEXT("downtown"), 1.2),
        FDistrictDef(TEXT("beach"), 1.4),
        FDistrictDef(TEXT("docks"), 0.7),
        FDistrictDef(TEXT("little_havana"), 0.9),
    };
}

int32 DistrictEconomy::DistrictCount() const
{
    return Entries.Num();
}

bool DistrictEconomy::HasDistrict(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> DistrictEconomy::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Entries.Num());
    for (const FEntry& Entry : Entries)
    {
        Out.Add(Entry.Id);
    }
    return Out;
}

double DistrictEconomy::BaseIndex(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry ? Entry->Base : -1.0;
}

double DistrictEconomy::ControlIn(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry ? Entry->Control : 0.0;
}

double DistrictEconomy::HeatIn(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry ? Entry->Heat : 0.0;
}

int32 DistrictEconomy::InvestmentIn(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry ? Entry->Investment : 0;
}

double DistrictEconomy::Desirability(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 1.0;
    }
    const double InvestBonus = InvestWeight * static_cast<double>(FMath::Min(Entry->Investment, InvestCap));
    const double Raw = Entry->Base + ControlWeight * Entry->Control + InvestBonus - HeatWeight * Entry->Heat;
    return FMath::Clamp(Raw, DesirMin, DesirMax);
}

int32 DistrictEconomy::PropertyValue(int32 BasePrice, const FString& Id) const
{
    return static_cast<int32>(FMath::RoundToDouble(static_cast<double>(BasePrice) * Desirability(Id)));
}

double DistrictEconomy::IncomeMultiplier(const FString& Id) const
{
    return Desirability(Id);
}

void DistrictEconomy::SetControl(const FString& Id, double Value)
{
    if (FEntry* Entry = Find(Id))
    {
        Entry->Control = FMath::Clamp(Value, 0.0, 1.0);
    }
}

void DistrictEconomy::AddHeat(const FString& Id, double Amount)
{
    if (Amount <= 0.0)
    {
        return;
    }
    if (FEntry* Entry = Find(Id))
    {
        Entry->Heat = FMath::Clamp(Entry->Heat + Amount, 0.0, 1.0);
    }
}

void DistrictEconomy::DecayHeat(const FString& Id, double Amount)
{
    if (Amount <= 0.0)
    {
        return;
    }
    if (FEntry* Entry = Find(Id))
    {
        Entry->Heat = FMath::Max(Entry->Heat - Amount, 0.0);
    }
}

void DistrictEconomy::DecayAllHeat(double Amount)
{
    for (FEntry& Entry : Entries)
    {
        DecayHeat(Entry.Id, Amount);
    }
}

void DistrictEconomy::Invest(const FString& Id)
{
    if (FEntry* Entry = Find(Id))
    {
        Entry->Investment += 1;
    }
}

void DistrictEconomy::Divest(const FString& Id)
{
    if (FEntry* Entry = Find(Id))
    {
        Entry->Investment = FMath::Max(Entry->Investment - 1, 0);
    }
}

void DistrictEconomy::Register(const FDistrictDef& Entry)
{
    if (Entry.Id.IsEmpty() || Index.Contains(Entry.Id) || Entry.Base <= 0.0)
    {
        return;
    }
    FEntry Stored;
    Stored.Id = Entry.Id;
    Stored.Base = Entry.Base;
    const int32 NewIndex = Entries.Add(Stored);
    Index.Add(Stored.Id, NewIndex);
}

const DistrictEconomy::FEntry* DistrictEconomy::Find(const FString& Id) const
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Entries[*FoundIndex] : nullptr;
}

DistrictEconomy::FEntry* DistrictEconomy::Find(const FString& Id)
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Entries[*FoundIndex] : nullptr;
}
