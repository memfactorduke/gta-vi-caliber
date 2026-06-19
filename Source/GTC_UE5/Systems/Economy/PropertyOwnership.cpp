// Copyright (c) 2026 GTC contributors

#include "PropertyOwnership.h"

PropertyOwnership::PropertyOwnership(const TArray<FPropertyDef>& InCatalogue)
{
    const TArray<FPropertyDef>& Source = InCatalogue.Num() > 0 ? InCatalogue : DefaultCatalogue();
    for (const FPropertyDef& Entry : Source)
    {
        Register(Entry);
    }
}

TArray<FPropertyDef> PropertyOwnership::DefaultCatalogue()
{
    return {
        FPropertyDef(TEXT("beach_condo"), TEXT("Ocean Drive Condo"), 25000, 0, true),
        FPropertyDef(TEXT("downtown_loft"), TEXT("Downtown Loft"), 90000, 0, true),
        FPropertyDef(TEXT("taxi_firm"), TEXT("Vice Taxi Firm"), 120000, 4000, false),
        FPropertyDef(TEXT("nightclub"), TEXT("Neon Nightclub"), 300000, 12000, false),
        FPropertyDef(TEXT("auto_shop"), TEXT("Auto Repair Shop"), 75000, 2500, false),
    };
}

int32 PropertyOwnership::PropertyCount() const
{
    return Catalogue.Num();
}

int32 PropertyOwnership::PriceOf(const FString& Id) const
{
    const FPropertyDef* Def = Find(Id);
    return Def ? Def->Price : -1;
}

int32 PropertyOwnership::IncomeOf(const FString& Id) const
{
    const FPropertyDef* Def = Find(Id);
    return Def ? Def->IncomePerDay : 0;
}

bool PropertyOwnership::IsSafehouse(const FString& Id) const
{
    const FPropertyDef* Def = Find(Id);
    return Def ? Def->bIsSafehouse : false;
}

bool PropertyOwnership::Owns(const FString& Id) const
{
    return OwnedSet.Contains(Id);
}

TArray<FString> PropertyOwnership::OwnedIds() const
{
    TArray<FString> Out = OwnedSet.Array();
    Out.Sort();
    return Out;
}

bool PropertyOwnership::HasSafehouse() const
{
    for (const FString& Id : OwnedSet)
    {
        const FPropertyDef* Def = Find(Id);
        if (Def && Def->bIsSafehouse)
        {
            return true;
        }
    }
    return false;
}

FString PropertyOwnership::NearestSafehouseOwned() const
{
    for (const FString& Id : OwnedIds())
    {
        const FPropertyDef* Def = Find(Id);
        if (Def && Def->bIsSafehouse)
        {
            return Id;
        }
    }
    return FString();
}

int32 PropertyOwnership::TotalInvested() const
{
    int32 Sum = 0;
    for (const FString& Id : OwnedSet)
    {
        const FPropertyDef* Def = Find(Id);
        if (Def)
        {
            Sum += Def->Price;
        }
    }
    return Sum;
}

int32 PropertyOwnership::DailyIncome() const
{
    int32 Sum = 0;
    for (const FString& Id : OwnedSet)
    {
        const FPropertyDef* Def = Find(Id);
        if (Def)
        {
            Sum += Def->IncomePerDay;
        }
    }
    return Sum;
}

FPropertyBuyResult PropertyOwnership::Buy(const FString& Id, int32 Balance)
{
    const FPropertyDef* Def = Find(Id);
    if (!Def)
    {
        return Fail(Balance, FString::Printf(TEXT("unknown property: %s"), *Id));
    }
    if (OwnedSet.Contains(Id))
    {
        return Fail(Balance, FString::Printf(TEXT("already owned: %s"), *Id));
    }
    const int32 Price = Def->Price;
    if (Balance < Price)
    {
        return Fail(Balance, FString::Printf(TEXT("insufficient funds: need %d, have %d"), Price, Balance));
    }
    OwnedSet.Add(Id);
    FPropertyBuyResult Result;
    Result.bSuccess = true;
    Result.Cost = Price;
    Result.NewBalance = Balance - Price;
    Result.Reason = FString();
    return Result;
}

void PropertyOwnership::Accrue(double DeltaDays)
{
    // Reject any non-finite span; NaN would poison the bank (every NaN compare is false).
    if (!FMath::IsFinite(DeltaDays) || DeltaDays <= 0.0)
    {
        return;
    }
    Pending += static_cast<double>(DailyIncome()) * DeltaDays;
}

double PropertyOwnership::PendingIncome() const
{
    return Pending;
}

int32 PropertyOwnership::Collect()
{
    if (!FMath::IsFinite(Pending))
    {
        Pending = 0.0;
        return 0;
    }
    const int32 Amount = static_cast<int32>(FMath::FloorToDouble(Pending));
    Pending -= static_cast<double>(Amount);
    return Amount;
}

FPropertySaveData PropertyOwnership::Serialize() const
{
    FPropertySaveData Data;
    Data.Owned = OwnedIds();
    Data.Pending = Pending;
    return Data;
}

void PropertyOwnership::Restore(const FPropertySaveData& Data)
{
    OwnedSet.Reset();
    Pending = FMath::Max(Data.Pending, 0.0);
    for (const FString& Id : Data.Owned)
    {
        if (Index.Contains(Id))
        {
            OwnedSet.Add(Id);
        }
    }
}

void PropertyOwnership::Reset()
{
    OwnedSet.Reset();
    Pending = 0.0;
}

FPropertyBuyResult PropertyOwnership::Fail(int32 Balance, const FString& Reason)
{
    FPropertyBuyResult Result;
    Result.bSuccess = false;
    Result.Cost = 0;
    Result.NewBalance = Balance;
    Result.Reason = Reason;
    return Result;
}

void PropertyOwnership::Register(const FPropertyDef& Entry)
{
    // Drop malformed rows: empty id or negative price. (the reference also drops non-int
    // price strings; those are unrepresentable with a typed FPropertyDef.)
    if (Entry.Id.IsEmpty() || Entry.Price < 0)
    {
        return;
    }
    FPropertyDef Stored = Entry;
    if (Stored.Name.IsEmpty())
    {
        Stored.Name = Entry.Id;
    }
    // Negative income falls back to 0 (Godot: income_int = income if int && >= 0 else 0).
    if (Stored.IncomePerDay < 0)
    {
        Stored.IncomePerDay = 0;
    }
    // the reference _catalogue[id] = {...} overwrites on a duplicate id (last-wins), keeping
    // the key's original insertion position. Mirror that: update in place if present.
    if (const int32* FoundIndex = Index.Find(Stored.Id))
    {
        Catalogue[*FoundIndex] = Stored;
        return;
    }
    const int32 NewIndex = Catalogue.Add(Stored);
    Index.Add(Stored.Id, NewIndex);
}

const FPropertyDef* PropertyOwnership::Find(const FString& Id) const
{
    const int32* FoundIndex = Index.Find(Id);
    return FoundIndex ? &Catalogue[*FoundIndex] : nullptr;
}
