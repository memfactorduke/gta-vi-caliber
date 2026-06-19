// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

struct FRandomStream;

/**
 * The car ROSTER — the canonical list of drivable cars in the game and the single
 * source of truth the rest of the vehicle stack was already keyed for. Every other
 * vehicle system addresses a car by an abstract id/class (FChopShop classes,
 * FVehicleCondition fuel rows, FVehicleHandling base top speed, AGTCTrafficVehicle's
 * footprint) but nothing actually DEFINED a car: name, class, how fast it goes, how
 * big it is, what it's worth, or which mesh/pawn renders it. This is that definition.
 *
 * Each FCarModel binds a designed game car to a real asset: the CitySampleVehicles
 * pack (Epic, free on Fab) at /Game/CitySampleVehicles. PawnBlueprintPath points at a
 * drivable ChaosVehicles WheeledVehiclePawn BP (BP_vehicleNN_<Type>_C) and BodyMeshPath
 * at its skeletal mesh; the adapter resolves those soft paths (TSoftClassPtr /
 * TSoftObjectPtr) — this struct stays pure data so it can be unit-tested headless and
 * carries no engine-vehicle dependency.
 *
 * What is VERIFIED here vs DESIGNED: the asset bindings (BP + mesh paths, and the
 * pack's NN/Type naming) are verified against the imported pack. The class mapping,
 * in-universe names, and the performance/economy/value numbers are a tunable first
 * pass — retune freely; only the paths must track the assets.
 *
 * Consumed by (the seams this closes):
 *   - FVehicleHandling::EffectiveTopSpeed  <- BaseTopSpeedCms()
 *   - AGTCTrafficVehicle::BodyHalfExtent / traffic bumper math <- BodyHalfExtent
 *   - FVehicleCondition fuel rows          <- TankLitres / EconomyLPerM (+ ClassId)
 *   - FChopShop fence value                <- ChopValue (+ ClassId matches its ids)
 *
 * PURE-CORE: double precision, FString/FVector only, no UObject/engine coupling.
 * Unit-tested headless (Vehicles/Catalog/Tests/CarCatalogTest.cpp, prefix
 * GTC.Vehicles.Catalog).
 */

/** Broad handling/value bracket a car falls in. The lowercase ClassId() of the first
 *  seven deliberately match FChopShop's class ids so a car's fence value lines up. */
enum class EGTCVehicleClass : uint8
{
    Compact,
    Sedan,
    Muscle,
    Sports,
    Super,
    Suv,
    Van,
    Truck,
    Bus,
};

/** One drivable car: identity + class + designed stats + the assets that render it. */
struct GTC_UE5_API FCarModel
{
    /** Stable catalogue id (kebab-case, e.g. "riptide"). Lookups key on this. */
    FString Id;
    /** In-universe display name shown in HUD/dealership/phone. */
    FString DisplayName;
    EGTCVehicleClass Class = EGTCVehicleClass::Sedan;

    // --- Asset bindings (soft paths into /Game/CitySampleVehicles) ------------------
    /** Drivable WheeledVehiclePawn BP class path, e.g.
     *  "/Game/CitySampleVehicles/vehicle12_Car/BP_vehicle12_Car.BP_vehicle12_Car_C". */
    FString PawnBlueprintPath;
    /** Body skeletal mesh path, e.g.
     *  "/Game/CitySampleVehicles/vehicle12_Car/Mesh/SKM_vehCar_vehicle12". */
    FString BodyMeshPath;

    // --- Driving feel ---------------------------------------------------------------
    double TopSpeedKmh = 0.0;   // marketing top speed
    double MassKg = 0.0;
    int32  Seats = 0;

    /** Body half-extents (cm): footprint for traffic bumper spacing + collision sizing. */
    FVector BodyHalfExtent = FVector(230.0, 95.0, 75.0);

    // --- Ownership / economy (feeds FVehicleCondition) ------------------------------
    double TankLitres = 0.0;
    double EconomyLPerM = 0.001;

    // --- Value (feeds FChopShop fence / dealership) ---------------------------------
    int32 ChopValue = 0;   // pristine fence value
    int32 Price = 0;       // showroom retail price

    /** Top speed expressed in cm/s for FVehicleHandling::EffectiveTopSpeed (engine cm). */
    double BaseTopSpeedCms() const { return TopSpeedKmh * (100000.0 / 3600.0); }
};

/** The roster: ordered, id-indexed, with a built-in default set of the imported cars. */
class GTC_UE5_API FGTCCarCatalog
{
public:
    /** Build from a roster; an empty list uses DefaultCars(). Entries with an empty id,
     *  empty PawnBlueprintPath, or a duplicate id are dropped. */
    explicit FGTCCarCatalog(const TArray<FCarModel>& Cars = TArray<FCarModel>());

    /** The built-in roster — the 13 CitySampleVehicles, as designed game cars. */
    static TArray<FCarModel> DefaultCars();

    int32 Num() const { return Entries.Num(); }
    bool Has(const FString& Id) const { return Index.Contains(Id); }

    /** Registered ids in roster order. */
    TArray<FString> Ids() const;

    /** The car for an id, or nullptr if unknown. */
    const FCarModel* Find(const FString& Id) const;

    /** All cars of a class, in roster order. */
    TArray<FCarModel> OfClass(EGTCVehicleClass InClass) const;

    /** A deterministic-per-seed pick from the whole roster (nullptr if empty). */
    const FCarModel* Random(FRandomStream& Rng) const;

    /** Lowercase class id; the first seven match FChopShop's class ids exactly. */
    static FString ClassId(EGTCVehicleClass InClass);

    /** Human-readable class label. */
    static const TCHAR* ClassDisplay(EGTCVehicleClass InClass);

private:
    void Register(const FCarModel& Car);

    TArray<FCarModel> Entries;     // roster order
    TMap<FString, int32> Index;    // id -> index into Entries
};
