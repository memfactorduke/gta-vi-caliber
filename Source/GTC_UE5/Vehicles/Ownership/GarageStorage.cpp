// Copyright (c) 2026 GTC contributors

#include "GarageStorage.h"

GarageStorage::GarageStorage(int32 InCapacityPerGarage)
{
    CapacityPerGarage = FMath::Max(1, InCapacityPerGarage);
}

bool GarageStorage::Store(const FString& GarageId, const FString& VehicleId)
{
    if (VehicleId.IsEmpty())
    {
        return false;
    }
    if (ImpoundedSet.Contains(VehicleId))
    {
        return false;
    }
    if (IsStored(VehicleId))
    {
        return false;
    }
    if (CountIn(GarageId) >= CapacityPerGarage)
    {
        return false;
    }
    FGarage& Garage = FindOrAddGarage(GarageId);
    Garage.Contents.Add(VehicleId);
    return true;
}

bool GarageStorage::Retrieve(const FString& GarageId, const FString& VehicleId)
{
    FGarage* Garage = FindGarage(GarageId);
    if (!Garage)
    {
        return false;
    }
    const int32 Idx = Garage->Contents.IndexOfByKey(VehicleId);
    if (Idx == INDEX_NONE)
    {
        return false;
    }
    Garage->Contents.RemoveAt(Idx);
    return true;
}

bool GarageStorage::IsStored(const FString& VehicleId) const
{
    return !GarageOf(VehicleId).IsEmpty();
}

FString GarageStorage::GarageOf(const FString& VehicleId) const
{
    for (const FGarage& Garage : Garages)
    {
        if (Garage.Contents.Contains(VehicleId))
        {
            return Garage.Id;
        }
    }
    return FString();
}

TArray<FString> GarageStorage::Contents(const FString& GarageId) const
{
    const FGarage* Garage = FindGarage(GarageId);
    return Garage ? Garage->Contents : TArray<FString>();
}

int32 GarageStorage::CountIn(const FString& GarageId) const
{
    const FGarage* Garage = FindGarage(GarageId);
    return Garage ? Garage->Contents.Num() : 0;
}

int32 GarageStorage::FreeSpace(const FString& GarageId) const
{
    return CapacityPerGarage - CountIn(GarageId);
}

int32 GarageStorage::Capacity() const
{
    return CapacityPerGarage;
}

int32 GarageStorage::TotalStored() const
{
    int32 Sum = 0;
    for (const FGarage& Garage : Garages)
    {
        Sum += Garage.Contents.Num();
    }
    return Sum;
}

void GarageStorage::Impound(const FString& VehicleId)
{
    if (VehicleId.IsEmpty())
    {
        return;
    }
    const FString GarageId = GarageOf(VehicleId);
    if (!GarageId.IsEmpty())
    {
        FGarage* Garage = FindGarage(GarageId);
        Garage->Contents.Remove(VehicleId);
    }
    if (!ImpoundedSet.Contains(VehicleId))
    {
        ImpoundedSet.Add(VehicleId);
        ImpoundedOrder.Add(VehicleId);
    }
}

bool GarageStorage::IsImpounded(const FString& VehicleId) const
{
    return ImpoundedSet.Contains(VehicleId);
}

GarageStorage::FRecoverResult GarageStorage::RecoverFromImpound(const FString& VehicleId, int32 Balance, int32 Fee)
{
    if (!ImpoundedSet.Contains(VehicleId))
    {
        FRecoverResult Result;
        Result.NewBalance = Balance;
        Result.Reason = FString::Printf(TEXT("not impounded: %s"), *VehicleId);
        return Result;
    }
    const int32 Cost = FMath::Max(0, Fee);
    if (Balance < Cost)
    {
        FRecoverResult Result;
        Result.NewBalance = Balance;
        Result.Reason = FString::Printf(TEXT("insufficient funds: need %d, have %d"), Cost, Balance);
        return Result;
    }

    RemoveImpound(VehicleId);

    FRecoverResult Result;
    Result.bSuccess = true;
    Result.Cost = Cost;
    Result.NewBalance = Balance - Cost;
    return Result;
}

void GarageStorage::Serialize(int32& OutCapacity, TArray<TPair<FString, TArray<FString>>>& OutGarages, TArray<FString>& OutImpounded) const
{
    OutCapacity = CapacityPerGarage;
    OutGarages.Reset();
    OutGarages.Reserve(Garages.Num());
    for (const FGarage& Garage : Garages)
    {
        OutGarages.Emplace(Garage.Id, Garage.Contents);
    }
    OutImpounded = ImpoundedOrder;
}

void GarageStorage::Restore(int32 InCapacity, const TArray<TPair<FString, TArray<FString>>>& InGarages, const TArray<FString>& InImpounded)
{
    Garages.Reset();
    GarageIndex.Reset();
    ImpoundedOrder.Reset();
    ImpoundedSet.Reset();

    if (InCapacity >= 1)
    {
        CapacityPerGarage = InCapacity;
    }

    for (const TPair<FString, TArray<FString>>& Pair : InGarages)
    {
        const FString& GarageId = Pair.Key;
        if (GarageId.IsEmpty())
        {
            continue;
        }
        TArray<FString> Clean;
        for (const FString& Entry : Pair.Value)
        {
            if (Entry.IsEmpty())
            {
                continue;
            }
            if (Clean.Num() >= CapacityPerGarage)
            {
                break;
            }
            Clean.Add(Entry);
        }
        // Drop empty garages so they don't resurrect lazily-created state.
        if (Clean.Num() == 0)
        {
            continue;
        }
        FGarage Garage;
        Garage.Id = GarageId;
        Garage.Contents = MoveTemp(Clean);
        const int32 NewIndex = Garages.Add(MoveTemp(Garage));
        GarageIndex.Add(GarageId, NewIndex);
    }

    for (const FString& Id : InImpounded)
    {
        if (Id.IsEmpty() || ImpoundedSet.Contains(Id))
        {
            continue;
        }
        ImpoundedSet.Add(Id);
        ImpoundedOrder.Add(Id);
    }
}

void GarageStorage::Reset()
{
    Garages.Reset();
    GarageIndex.Reset();
    ImpoundedOrder.Reset();
    ImpoundedSet.Reset();
}

const GarageStorage::FGarage* GarageStorage::FindGarage(const FString& GarageId) const
{
    const int32* Idx = GarageIndex.Find(GarageId);
    return Idx ? &Garages[*Idx] : nullptr;
}

GarageStorage::FGarage* GarageStorage::FindGarage(const FString& GarageId)
{
    const int32* Idx = GarageIndex.Find(GarageId);
    return Idx ? &Garages[*Idx] : nullptr;
}

GarageStorage::FGarage& GarageStorage::FindOrAddGarage(const FString& GarageId)
{
    if (FGarage* Existing = FindGarage(GarageId))
    {
        return *Existing;
    }
    FGarage Garage;
    Garage.Id = GarageId;
    const int32 NewIndex = Garages.Add(MoveTemp(Garage));
    GarageIndex.Add(GarageId, NewIndex);
    return Garages[NewIndex];
}

void GarageStorage::RemoveImpound(const FString& VehicleId)
{
    ImpoundedSet.Remove(VehicleId);
    ImpoundedOrder.Remove(VehicleId);
}
