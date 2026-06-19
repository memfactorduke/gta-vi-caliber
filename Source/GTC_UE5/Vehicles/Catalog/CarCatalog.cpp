// Copyright Epic Games, Inc. All Rights Reserved.

#include "CarCatalog.h"
#include "Math/RandomStream.h"

namespace
{
    /** Per-class stat preset. A car inherits these, then overrides identity + assets. */
    struct FClassPreset
    {
        double TopSpeedKmh;
        double MassKg;
        int32  Seats;
        FVector HalfExtent;   // cm
        double TankLitres;
        double EconomyLPerM;
        int32  ChopValue;     // pristine fence value
        int32  Price;         // showroom retail
    };

    FClassPreset PresetFor(EGTCVehicleClass C)
    {
        switch (C)
        {
        case EGTCVehicleClass::Compact: return {165.0, 1200.0, 4, FVector(210.0,  90.0,  70.0),  45.0, 0.0008,   8000,  18000};
        case EGTCVehicleClass::Sedan:   return {190.0, 1500.0, 4, FVector(235.0,  95.0,  73.0),  60.0, 0.0009,  15000,  32000};
        case EGTCVehicleClass::Muscle:  return {230.0, 1700.0, 4, FVector(245.0, 100.0,  70.0),  75.0, 0.0016,  35000,  78000};
        case EGTCVehicleClass::Sports:  return {270.0, 1400.0, 2, FVector(235.0,  98.0,  62.0),  70.0, 0.0014,  60000, 145000};
        case EGTCVehicleClass::Super:   return {330.0, 1350.0, 2, FVector(240.0, 100.0,  58.0),  75.0, 0.0018, 150000, 850000};
        case EGTCVehicleClass::Suv:     return {185.0, 2300.0, 5, FVector(255.0, 105.0,  90.0),  80.0, 0.0013,  22000,  55000};
        case EGTCVehicleClass::Van:     return {165.0, 2200.0, 3, FVector(280.0, 110.0, 105.0),  80.0, 0.0013,  18000,  38000};
        case EGTCVehicleClass::Truck:   return {150.0, 3500.0, 2, FVector(320.0, 120.0, 120.0), 150.0, 0.0028,  28000,  90000};
        case EGTCVehicleClass::Bus:     return {130.0,12000.0,30, FVector(600.0, 130.0, 160.0), 300.0, 0.0045,  40000, 180000};
        default:                        return {190.0, 1500.0, 4, FVector(235.0,  95.0,  73.0),  60.0, 0.0009,  15000,  32000};
        }
    }

    /** Build a car: class preset + identity + asset paths derived from the pack layout.
     *  Stem is the per-vehicle folder/file stem, e.g. "vehicle12_Car"; MeshType is the
     *  pack's mesh infix, e.g. "Car"/"Van"/"Truck"/"Bus"; NN is the zero-padded index.
     *  MeshStem/MeshNN override where the BODY MESH comes from when a BP is a recolor
     *  variant that reuses another vehicle's mesh (vehicles 12 & 13 reuse vehicle03);
     *  pass nullptr to use this car's own Stem/NN. The pawn BP always uses Stem. */
    FCarModel MakeCar(const TCHAR* Id, const TCHAR* Name, EGTCVehicleClass Class,
        const TCHAR* Stem, const TCHAR* MeshType, const TCHAR* NN,
        const TCHAR* MeshStem = nullptr, const TCHAR* MeshNN = nullptr)
    {
        const FClassPreset P = PresetFor(Class);
        const TCHAR* BodyStem = MeshStem ? MeshStem : Stem;
        const TCHAR* BodyNN = MeshNN ? MeshNN : NN;
        FCarModel Car;
        Car.Id = Id;
        Car.DisplayName = Name;
        Car.Class = Class;
        Car.PawnBlueprintPath = FString::Printf(
            TEXT("/Game/CitySampleVehicles/%s/BP_%s.BP_%s_C"), Stem, Stem, Stem);
        Car.BodyMeshPath = FString::Printf(
            TEXT("/Game/CitySampleVehicles/%s/Mesh/SKM_veh%s_vehicle%s"), BodyStem, MeshType, BodyNN);
        Car.TopSpeedKmh = P.TopSpeedKmh;
        Car.MassKg = P.MassKg;
        Car.Seats = P.Seats;
        Car.BodyHalfExtent = P.HalfExtent;
        Car.TankLitres = P.TankLitres;
        Car.EconomyLPerM = P.EconomyLPerM;
        Car.ChopValue = P.ChopValue;
        Car.Price = P.Price;
        return Car;
    }
}

TArray<FCarModel> FGTCCarCatalog::DefaultCars()
{
    // The 13 drivable CitySampleVehicles (vehicle08's separate trailer is towed cargo,
    // not a standalone car, so it is intentionally excluded). Class/name/stats are a
    // tunable design pass; the BP + mesh paths track the imported pack.
    TArray<FCarModel> Out;
    using EC = EGTCVehicleClass;
    Out.Add(MakeCar(TEXT("cargo-cay"),  TEXT("Cargo Cay"),  EC::Van,     TEXT("vehicle01_Van"),   TEXT("Van"),   TEXT("01")));
    Out.Add(MakeCar(TEXT("surfline"),   TEXT("Surfline"),   EC::Sedan,   TEXT("vehicle02_Car"),   TEXT("Car"),   TEXT("02")));
    Out.Add(MakeCar(TEXT("tidewater"),  TEXT("Tidewater"),  EC::Compact, TEXT("vehicle03_Car"),   TEXT("Car"),   TEXT("03")));
    Out.Add(MakeCar(TEXT("dockhand"),   TEXT("Dockhand"),   EC::Truck,   TEXT("vehicle04_Truck"), TEXT("Truck"), TEXT("04")));
    Out.Add(MakeCar(TEXT("marlin-gt"),  TEXT("Marlin GT"),  EC::Sports,  TEXT("vehicle05_Car"),   TEXT("Car"),   TEXT("05")));
    Out.Add(MakeCar(TEXT("boulevard"),  TEXT("Boulevard"),  EC::Muscle,  TEXT("vehicle06_Car"),   TEXT("Car"),   TEXT("06")));
    Out.Add(MakeCar(TEXT("esplanade"),  TEXT("Esplanade"),  EC::Sedan,   TEXT("vehicle07_Car"),   TEXT("Car"),   TEXT("07")));
    Out.Add(MakeCar(TEXT("haulmaster"), TEXT("Haulmaster"), EC::Truck,   TEXT("vehicle08_Truck"), TEXT("Truck"), TEXT("08")));
    Out.Add(MakeCar(TEXT("harbor-cay"), TEXT("Harbor Cay"), EC::Van,     TEXT("vehicle09_Van"),   TEXT("Van"),   TEXT("09")));
    Out.Add(MakeCar(TEXT("causeway"),   TEXT("Causeway"),   EC::Bus,     TEXT("vehicle10_Bus"),   TEXT("Bus"),   TEXT("10")));
    Out.Add(MakeCar(TEXT("quaymaster"), TEXT("Quaymaster"), EC::Truck,   TEXT("vehicle11_Truck"), TEXT("Truck"), TEXT("11")));
    // vehicles 12 & 13 are recolor variants reusing vehicle03's body mesh.
    Out.Add(MakeCar(TEXT("riptide"),    TEXT("Riptide"),    EC::Sports,  TEXT("vehicle12_Car"),   TEXT("Car"),   TEXT("12"), TEXT("vehicle03_Car"), TEXT("03")));
    Out.Add(MakeCar(TEXT("coral-ss"),   TEXT("Coral SS"),   EC::Super,   TEXT("vehicle13_Car"),   TEXT("Car"),   TEXT("13"), TEXT("vehicle03_Car"), TEXT("03")));
    return Out;
}

FGTCCarCatalog::FGTCCarCatalog(const TArray<FCarModel>& Cars)
{
    const TArray<FCarModel>& Source = Cars.Num() > 0 ? Cars : DefaultCars();
    for (const FCarModel& Car : Source)
    {
        Register(Car);
    }
}

void FGTCCarCatalog::Register(const FCarModel& Car)
{
    if (Car.Id.IsEmpty() || Car.PawnBlueprintPath.IsEmpty())
    {
        return;  // malformed: no id or nothing to spawn
    }
    if (Index.Contains(Car.Id))
    {
        return;  // duplicate id: first wins
    }
    Index.Add(Car.Id, Entries.Num());
    Entries.Add(Car);
}

TArray<FString> FGTCCarCatalog::Ids() const
{
    TArray<FString> Out;
    Out.Reserve(Entries.Num());
    for (const FCarModel& Car : Entries)
    {
        Out.Add(Car.Id);
    }
    return Out;
}

const FCarModel* FGTCCarCatalog::Find(const FString& Id) const
{
    const int32* Idx = Index.Find(Id);
    return Idx ? &Entries[*Idx] : nullptr;
}

TArray<FCarModel> FGTCCarCatalog::OfClass(EGTCVehicleClass InClass) const
{
    TArray<FCarModel> Out;
    for (const FCarModel& Car : Entries)
    {
        if (Car.Class == InClass)
        {
            Out.Add(Car);
        }
    }
    return Out;
}

const FCarModel* FGTCCarCatalog::Random(FRandomStream& Rng) const
{
    if (Entries.Num() == 0)
    {
        return nullptr;
    }
    return &Entries[Rng.RandRange(0, Entries.Num() - 1)];
}

FString FGTCCarCatalog::ClassId(EGTCVehicleClass InClass)
{
    switch (InClass)
    {
    case EGTCVehicleClass::Compact: return TEXT("compact");
    case EGTCVehicleClass::Sedan:   return TEXT("sedan");
    case EGTCVehicleClass::Muscle:  return TEXT("muscle");
    case EGTCVehicleClass::Sports:  return TEXT("sports");
    case EGTCVehicleClass::Super:   return TEXT("super");
    case EGTCVehicleClass::Suv:     return TEXT("suv");
    case EGTCVehicleClass::Van:     return TEXT("van");
    case EGTCVehicleClass::Truck:   return TEXT("truck");
    case EGTCVehicleClass::Bus:     return TEXT("bus");
    default:                        return TEXT("sedan");
    }
}

const TCHAR* FGTCCarCatalog::ClassDisplay(EGTCVehicleClass InClass)
{
    switch (InClass)
    {
    case EGTCVehicleClass::Compact: return TEXT("Compact");
    case EGTCVehicleClass::Sedan:   return TEXT("Sedan");
    case EGTCVehicleClass::Muscle:  return TEXT("Muscle");
    case EGTCVehicleClass::Sports:  return TEXT("Sports");
    case EGTCVehicleClass::Super:   return TEXT("Supercar");
    case EGTCVehicleClass::Suv:     return TEXT("SUV");
    case EGTCVehicleClass::Van:     return TEXT("Van");
    case EGTCVehicleClass::Truck:   return TEXT("Truck");
    case EGTCVehicleClass::Bus:     return TEXT("Bus");
    default:                        return TEXT("Sedan");
    }
}
