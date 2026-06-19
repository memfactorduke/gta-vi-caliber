// Copyright Epic Games, Inc. All Rights Reserved.

#include "GangTerritory.h"

const FString GangTerritory::PlayerOwner = TEXT("player");

GangTerritory::GangTerritory(const TArray<FDistrictDef>& Districts)
{
    const TArray<FDistrictDef>& Source = Districts.Num() > 0 ? Districts : DefaultDistricts();
    for (const FDistrictDef& Def : Source)
    {
        Register(Def);
    }
}

TArray<GangTerritory::FDistrictDef> GangTerritory::DefaultDistricts()
{
    return {
        FDistrictDef(TEXT("downtown"), TEXT("coast_kings")),
        FDistrictDef(TEXT("beach"), TEXT("biscayne_set")),
        FDistrictDef(TEXT("docks"), TEXT("marina_cartel")),
        FDistrictDef(TEXT("little_havana"), TEXT("coast_kings")),
    };
}

int32 GangTerritory::DistrictCount() const
{
    return Entries.Num();
}

bool GangTerritory::HasDistrict(const FString& DistrictId) const
{
    return Index.Contains(DistrictId);
}

FString GangTerritory::OwnerOf(const FString& DistrictId) const
{
    const FEntry* Entry = Find(DistrictId);
    return Entry ? Entry->Owner : FString();
}

double GangTerritory::InfluenceIn(const FString& DistrictId) const
{
    const FEntry* Entry = Find(DistrictId);
    return Entry ? Entry->Influence : 0.0;
}

void GangTerritory::AddInfluence(const FString& DistrictId, double Amount)
{
    if (!Index.Contains(DistrictId) || Amount <= 0.0)
    {
        return;
    }
    SetInfluence(DistrictId, InfluenceIn(DistrictId) + Amount);
}

void GangTerritory::LoseInfluence(const FString& DistrictId, double Amount)
{
    if (!Index.Contains(DistrictId) || Amount <= 0.0)
    {
        return;
    }
    SetInfluence(DistrictId, InfluenceIn(DistrictId) - Amount);
}

bool GangTerritory::IsContested(const FString& DistrictId, double Threshold) const
{
    return InfluenceIn(DistrictId) > Threshold;
}

bool GangTerritory::TakeOver(const FString& DistrictId)
{
    FEntry* Entry = Find(DistrictId);
    if (!Entry)
    {
        return false;
    }
    if (Entry->Influence < 1.0)
    {
        return false;
    }
    if (Entry->Owner == PlayerOwner)
    {
        return false;
    }
    Entry->Owner = PlayerOwner;
    return true;
}

TArray<FString> GangTerritory::PlayerDistricts() const
{
    TArray<FString> Out;
    for (const FEntry& Entry : Entries)
    {
        if (Entry.Owner == PlayerOwner)
        {
            Out.Add(Entry.Id);
        }
    }
    return Out;
}

double GangTerritory::ControlledFraction() const
{
    if (Entries.Num() == 0)
    {
        return 0.0;
    }
    return static_cast<double>(PlayerDistricts().Num()) / static_cast<double>(Entries.Num());
}

bool GangTerritory::AllOwned() const
{
    return Entries.Num() > 0 && PlayerDistricts().Num() == Entries.Num();
}

TArray<GangTerritory::FDistrictDef> GangTerritory::Serialize() const
{
    TArray<FDistrictDef> Out;
    Out.Reserve(Entries.Num());
    for (const FEntry& Entry : Entries)
    {
        FDistrictDef Def;
        Def.Id = Entry.Id;
        Def.Owner = Entry.Owner;
        Def.HomeOwner = Entry.HomeOwner;
        Def.bHasHomeOwner = true;
        Def.Influence = Entry.Influence;
        Out.Add(Def);
    }
    return Out;
}

void GangTerritory::Restore(const TArray<FDistrictDef>& Districts)
{
    Entries.Reset();
    Index.Reset();
    for (const FDistrictDef& Def : Districts)
    {
        Register(Def);
        if (Index.Contains(Def.Id))
        {
            SetInfluence(Def.Id, Def.Influence);
        }
    }
}

void GangTerritory::RestoreEmpty()
{
    Entries.Reset();
    Index.Reset();
}

void GangTerritory::Reset()
{
    for (FEntry& Entry : Entries)
    {
        Entry.Owner = Entry.HomeOwner;
        Entry.Influence = 0.0;
    }
}

void GangTerritory::Register(const FDistrictDef& Def)
{
    const FString Id = Def.Id;
    if (Id.IsEmpty() || Index.Contains(Id))
    {
        return;
    }
    FEntry Entry;
    Entry.Id = Id;
    Entry.Owner = Def.Owner;
    // home_owner = dict.get("home_owner", owner): default to owner when unset.
    Entry.HomeOwner = Def.bHasHomeOwner ? Def.HomeOwner : Def.Owner;
    Entry.Influence = FMath::Clamp(Def.Influence, 0.0, 1.0);
    const int32 NewIndex = Entries.Add(Entry);
    Index.Add(Id, NewIndex);
}

void GangTerritory::SetInfluence(const FString& DistrictId, double Value)
{
    FEntry* Entry = Find(DistrictId);
    if (Entry)
    {
        Entry->Influence = FMath::Clamp(Value, 0.0, 1.0);
    }
}

const GangTerritory::FEntry* GangTerritory::Find(const FString& Id) const
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}

GangTerritory::FEntry* GangTerritory::Find(const FString& Id)
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}
