// Copyright (c) 2026 GTC contributors

#include "VehicleCondition.h"

FVehicleCondition::FVehicleCondition(const TArray<FVehicleDef>& Vehicles)
{
    const TArray<FVehicleDef>& Source = Vehicles.Num() > 0 ? Vehicles : DefaultVehicles();
    for (const FVehicleDef& Def : Source)
    {
        Register(Def);
    }
}

TArray<FVehicleCondition::FVehicleDef> FVehicleCondition::DefaultVehicles()
{
    TArray<FVehicleDef> Out;
    Out.Add({TEXT("sedan"), 60.0, 0.0009, 0.0, 0.0});
    Out.Add({TEXT("sports"), 70.0, 0.0014, 0.0, 0.0});
    Out.Add({TEXT("bike"), 18.0, 0.0005, 0.0, 0.0});
    Out.Add({TEXT("muscle"), 75.0, 0.0016, 0.0, 0.0});
    return Out;
}

int32 FVehicleCondition::VehicleCount() const
{
    return Entries.Num();
}

bool FVehicleCondition::HasVehicle(const FString& Id) const
{
    return Index.Contains(Id);
}

TArray<FString> FVehicleCondition::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Entries.Num());
    for (const FVehicleEntry& Entry : Entries)
    {
        Out.Add(Entry.Id);
    }
    return Out;
}

double FVehicleCondition::FuelOf(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    return Entry ? Entry->Fuel : 0.0;
}

double FVehicleCondition::FuelFraction(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 0.0;
    }
    if (Entry->Tank <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(Entry->Fuel / Entry->Tank, 0.0, 1.0);
}

double FVehicleCondition::EngineWearOf(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    return Entry ? Entry->EngineWear : 0.0;
}

double FVehicleCondition::TireWearOf(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    return Entry ? Entry->TireWear : 0.0;
}

double FVehicleCondition::Condition(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 1.0;
    }
    const double EngineOk = 1.0 - Entry->EngineWear;
    const double TireOk = 1.0 - Entry->TireWear;
    return FMath::Clamp(
        EngineConditionWeight * EngineOk + (1.0 - EngineConditionWeight) * TireOk, 0.0, 1.0);
}

bool FVehicleCondition::IsOutOfFuel(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    return Entry != nullptr && Entry->Fuel <= 0.0;
}

double FVehicleCondition::TopSpeedFactor(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 1.0;
    }
    if (Entry->Fuel <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(
        EngineFloor + (1.0 - EngineFloor) * (1.0 - Entry->EngineWear), EngineFloor, 1.0);
}

double FVehicleCondition::GripFactor(const FString& Id) const
{
    const FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 1.0;
    }
    return FMath::Clamp(TireFloor + (1.0 - TireFloor) * (1.0 - Entry->TireWear), TireFloor, 1.0);
}

FVehicleCondition::FDriveResult FVehicleCondition::Drive(const FString& Id, double DistanceM, double Intensity)
{
    FDriveResult Result;
    FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        // Neutral dict for an unknown vehicle.
        return Result;
    }
    if (DistanceM <= 0.0)
    {
        Result.bStalled = Entry->Fuel <= 0.0;
        Result.EngineWear = Entry->EngineWear;
        Result.TireWear = Entry->TireWear;
        return Result;
    }
    // A stalled (empty-tank) car does not move, so it neither burns fuel nor wears.
    if (Entry->Fuel <= 0.0)
    {
        Result.bStalled = true;
        Result.EngineWear = Entry->EngineWear;
        Result.TireWear = Entry->TireWear;
        return Result;
    }
    const double Inten = FMath::Max(Intensity, MinIntensity);
    const double Effort = DistanceM * Inten;
    const double WantFuel = Entry->Economy * Effort;
    const double FuelUsed = FMath::Min(WantFuel, Entry->Fuel);
    Entry->Fuel = FMath::Max(Entry->Fuel - WantFuel, 0.0);
    // Wear scales with the distance ACTUALLY driven: if the tank runs dry partway, the car only
    // travels the fuelled fraction, so it shouldn't wear for the full requested distance.
    const double TravelFraction = WantFuel <= 0.0 ? 1.0 : FuelUsed / WantFuel;
    const double Driven = Effort * TravelFraction;
    Entry->EngineWear = FMath::Clamp(Entry->EngineWear + EngineWearPerM * Driven, 0.0, 1.0);
    Entry->TireWear = FMath::Clamp(Entry->TireWear + TireWearPerM * Driven, 0.0, 1.0);
    Result.FuelUsed = FuelUsed;
    Result.bStalled = Entry->Fuel <= 0.0;
    Result.EngineWear = Entry->EngineWear;
    Result.TireWear = Entry->TireWear;
    return Result;
}

void FVehicleCondition::ApplyCrash(const FString& Id, double Severity)
{
    FVehicleEntry* Entry = Find(Id);
    if (!Entry || Severity <= 0.0)
    {
        return;
    }
    Entry->EngineWear = FMath::Clamp(
        Entry->EngineWear + CrashEngineWear * FMath::Clamp(Severity, 0.0, 1.0), 0.0, 1.0);
}

double FVehicleCondition::Refuel(const FString& Id, double Liters)
{
    FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        return 0.0;
    }
    const double Space = FMath::Max(Entry->Tank - Entry->Fuel, 0.0);
    const double Add = Liters < 0.0 ? Space : FMath::Clamp(Liters, 0.0, Space);
    Entry->Fuel = Entry->Fuel + Add;
    return Add;
}

void FVehicleCondition::Service(const FString& Id, bool bEngine, bool bTires)
{
    FVehicleEntry* Entry = Find(Id);
    if (!Entry)
    {
        return;
    }
    if (bEngine)
    {
        Entry->EngineWear = 0.0;
    }
    if (bTires)
    {
        Entry->TireWear = 0.0;
    }
}

TArray<FVehicleCondition::FVehicleSave> FVehicleCondition::ToState() const
{
    TArray<FVehicleSave> Out;
    Out.Reserve(Entries.Num());
    for (const FVehicleEntry& Entry : Entries)
    {
        FVehicleSave Save;
        Save.Id = Entry.Id;
        Save.Fuel = Entry.Fuel;
        Save.EngineWear = Entry.EngineWear;
        Save.TireWear = Entry.TireWear;
        Out.Add(Save);
    }
    return Out;
}

void FVehicleCondition::LoadState(const TArray<FVehicleSave>& Data)
{
    for (const FVehicleSave& Row : Data)
    {
        FVehicleEntry* Entry = Find(Row.Id);
        if (!Entry)
        {
            continue;
        }
        if (Row.bHasFuel)
        {
            Entry->Fuel = FMath::Clamp(Row.Fuel, 0.0, Entry->Tank);
        }
        if (Row.bHasEngineWear)
        {
            Entry->EngineWear = FMath::Clamp(Row.EngineWear, 0.0, 1.0);
        }
        if (Row.bHasTireWear)
        {
            Entry->TireWear = FMath::Clamp(Row.TireWear, 0.0, 1.0);
        }
    }
}

void FVehicleCondition::Register(const FVehicleDef& Def)
{
    if (Def.Id.IsEmpty() || Index.Contains(Def.Id))
    {
        return;
    }
    if (Def.Tank <= 0.0)
    {
        return;
    }
    FVehicleEntry Entry;
    Entry.Id = Def.Id;
    Entry.Tank = Def.Tank;
    Entry.Economy = Def.Economy > 0.0 ? Def.Economy : DefaultEconomy;
    Entry.Fuel = Def.Tank;
    Entry.EngineWear = FMath::Clamp(Def.EngineWear, 0.0, 1.0);
    Entry.TireWear = FMath::Clamp(Def.TireWear, 0.0, 1.0);
    const int32 NewIndex = Entries.Add(Entry);
    Index.Add(Def.Id, NewIndex);
}

const FVehicleCondition::FVehicleEntry* FVehicleCondition::Find(const FString& Id) const
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}

FVehicleCondition::FVehicleEntry* FVehicleCondition::Find(const FString& Id)
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}
