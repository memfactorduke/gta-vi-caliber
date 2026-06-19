// Copyright (c) 2026 GTC contributors

#include "ChopShop.h"

FChopShop::FChopShop(const TArray<FClassDef>& Classes)
{
    const TArray<FClassDef>& Source = Classes.Num() > 0 ? Classes : DefaultClasses();
    for (const FClassDef& Def : Source)
    {
        Register(Def);
    }
}

TArray<FChopShop::FClassDef> FChopShop::DefaultClasses()
{
    TArray<FClassDef> Out;
    Out.Add({TEXT("compact"), 8000});
    Out.Add({TEXT("sedan"), 15000});
    Out.Add({TEXT("bike"), 12000});
    Out.Add({TEXT("suv"), 22000});
    Out.Add({TEXT("muscle"), 35000});
    Out.Add({TEXT("sports"), 60000});
    Out.Add({TEXT("super"), 150000});
    return Out;
}

int32 FChopShop::ClassCount() const
{
    return Entries.Num();
}

bool FChopShop::HasClass(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> FChopShop::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Entries.Num());
    for (const FClassEntry& Entry : Entries)
    {
        Out.Add(Entry.Id);
    }
    return Out;
}

int32 FChopShop::BaseValueOf(const FString& Id) const
{
    const FClassEntry* Entry = Find(Id);
    return Entry ? Entry->Base : -1;
}

int32 FChopShop::Value(const FString& Id, double Condition) const
{
    const FClassEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 0;
    }
    const double Cond = FMath::Clamp(Condition, 0.0, 1.0);
    const double ConditionFactor = ScrapFloor + (1.0 - ScrapFloor) * Cond;
    const double Demand = IsRequested(Id) ? RequestBonus : 1.0;
    return FMath::RoundToInt32(static_cast<double>(Entry->Base) * ConditionFactor * Demand);
}

bool FChopShop::IsRequested(const FString& Id) const
{
    return Requests.Contains(Id);
}

TArray<FString> FChopShop::Requested() const
{
    return Requests;
}

void FChopShop::SetRequests(const TArray<FString>& ClassIds)
{
    Requests.Reset();
    for (const FString& Raw : ClassIds)
    {
        if (Index.Contains(Raw) && !Requests.Contains(Raw))
        {
            Requests.Add(Raw);
        }
    }
}

void FChopShop::RotateRequests(FRandomStream& Rng, int32 Count)
{
    TArray<FString> Pool = Ids();
    ShuffleIds(Rng, Pool);
    Requests.Reset();
    const int32 Take = FMath::Min(FMath::Max(Count, 0), Pool.Num());
    for (int32 i = 0; i < Take; ++i)
    {
        Requests.Add(Pool[i]);
    }
}

void FChopShop::RotateRequestsNoRng()
{
    // Mirrors the reference `if rng == null: return` — no rotation without an rng.
}

FChopShop::FDeliverResult FChopShop::Deliver(const FString& Id, double Condition, bool bHot)
{
    FDeliverResult Result;
    if (!Index.Contains(Id))
    {
        Result.Reason = TEXT("unknown class");
        return Result;
    }
    const bool bWasRequested = IsRequested(Id);
    int32 Payout = Value(Id, Condition);
    if (bHot)
    {
        Payout = FMath::RoundToInt32(static_cast<double>(Payout) * (1.0 - HeatDiscount));
    }
    Requests.Remove(Id);
    Earned += Payout;
    Deliveries += 1;
    Result.bAccepted = true;
    Result.Payout = Payout;
    Result.bWasRequested = bWasRequested;
    Result.Reason = TEXT("");
    return Result;
}

int32 FChopShop::TotalEarned() const
{
    return Earned;
}

int32 FChopShop::DeliveriesCount() const
{
    return Deliveries;
}

void FChopShop::ShuffleIds(FRandomStream& Rng, TArray<FString>& Arr)
{
    for (int32 i = Arr.Num() - 1; i > 0; --i)
    {
        const int32 j = Rng.RandHelper(i + 1);
        Arr.Swap(i, j);
    }
}

void FChopShop::Register(const FClassDef& Def)
{
    if (Def.Id.IsEmpty() || Index.Contains(Def.Id) || Def.Base <= 0)
    {
        return;
    }
    FClassEntry Entry;
    Entry.Id = Def.Id;
    Entry.Base = Def.Base;
    const int32 NewIndex = Entries.Add(Entry);
    Index.Add(Def.Id, NewIndex);
}

const FChopShop::FClassEntry* FChopShop::Find(const FString& Id) const
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}
