// Copyright (c) 2026 GTC contributors

#include "FactionStanding.h"

FactionStanding::FactionStanding(const TArray<FFactionDef>& Factions)
{
    const TArray<FFactionDef>& Source = Factions.Num() > 0 ? Factions : DefaultFactions();
    for (const FFactionDef& Def : Source)
    {
        Register(Def);
    }
}

TArray<FactionStanding::FFactionDef> FactionStanding::DefaultFactions()
{
    return {
        FFactionDef(TEXT("coast_kings"), TEXT("marina_cartel")),
        FFactionDef(TEXT("marina_cartel"), TEXT("coast_kings")),
        FFactionDef(TEXT("biscayne_set"), TEXT("")),
        FFactionDef(TEXT("police"), TEXT("")),
    };
}

int32 FactionStanding::FactionCount() const
{
    return Entries.Num();
}

bool FactionStanding::HasFaction(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> FactionStanding::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Entries.Num());
    for (const FEntry& Entry : Entries)
    {
        Out.Add(Entry.Id);
    }
    return Out;
}

FString FactionStanding::RivalOf(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry ? Entry->Rival : FString();
}

int32 FactionStanding::StandingOf(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry ? Entry->Standing : 0;
}

FString FactionStanding::TierOf(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    if (!Entry)
    {
        return FString();
    }
    const int32 S = Entry->Standing;
    if (S <= HostileAt)
    {
        return TEXT("hostile");
    }
    if (S <= WaryAt)
    {
        return TEXT("wary");
    }
    if (S < FriendlyAt)
    {
        return TEXT("neutral");
    }
    if (S < AlliedAt)
    {
        return TEXT("friendly");
    }
    return TEXT("allied");
}

bool FactionStanding::IsHostile(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry != nullptr && Entry->Standing <= HostileAt;
}

bool FactionStanding::IsAllied(const FString& Id) const
{
    const FEntry* Entry = Find(Id);
    return Entry != nullptr && Entry->Standing >= AlliedAt;
}

bool FactionStanding::WillAttack(const FString& Id) const
{
    return IsHostile(Id);
}

bool FactionStanding::WillAssist(const FString& Id) const
{
    return IsAllied(Id);
}

void FactionStanding::SetStanding(const FString& Id, int32 Value)
{
    FEntry* Entry = Find(Id);
    if (Entry)
    {
        Entry->Standing = FMath::Clamp(Value, MinStanding, MaxStanding);
    }
}

void FactionStanding::Adjust(const FString& Id, int32 Delta, double Spillover)
{
    FEntry* Entry = Find(Id);
    if (!Entry)
    {
        return;
    }
    SetStanding(Id, Entry->Standing + Delta);

    // Re-find: SetStanding does not invalidate Entries, but read rival explicitly.
    const FString Rival = Entry->Rival;
    if (!Rival.IsEmpty() && Spillover > 0.0 && Index.Contains(Rival))
    {
        const int32 Bleed = FMath::RoundToInt(static_cast<double>(Delta) * FMath::Clamp(Spillover, 0.0, 1.0));
        const FEntry* RivalEntry = Find(Rival);
        SetStanding(Rival, RivalEntry->Standing - Bleed);
    }
}

TArray<TPair<FString, int32>> FactionStanding::ToMap() const
{
    TArray<TPair<FString, int32>> Out;
    Out.Reserve(Entries.Num());
    for (const FEntry& Entry : Entries)
    {
        Out.Emplace(Entry.Id, Entry.Standing);
    }
    return Out;
}

void FactionStanding::LoadMap(const TArray<TPair<FString, int32>>& Data)
{
    for (const TPair<FString, int32>& Pair : Data)
    {
        if (Index.Contains(Pair.Key))
        {
            SetStanding(Pair.Key, Pair.Value);
        }
    }
}

void FactionStanding::Register(const FFactionDef& Def)
{
    const FString Id = Def.Id;
    if (Id.IsEmpty() || Index.Contains(Id))
    {
        return;
    }
    FEntry Entry;
    Entry.Id = Id;
    Entry.Rival = Def.Rival;
    Entry.Standing = 0;
    const int32 NewIndex = Entries.Add(Entry);
    Index.Add(Id, NewIndex);
}

const FactionStanding::FEntry* FactionStanding::Find(const FString& Id) const
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}

FactionStanding::FEntry* FactionStanding::Find(const FString& Id)
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}
