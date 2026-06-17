// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure per-vehicle wear/fuel ledger — the persistent ownership layer. Each registered vehicle
 * carries fuel (litres), engine wear and tire wear (each 0 new .. 1 shot). Driving a distance
 * burns fuel by the vehicle's economy and accrues engine + tire wear (faster under hard driving);
 * a crash bumps engine wear. From that it derives a single overall condition (0..1) — exactly the
 * float FChopShop::Value() consumes — plus drivability gates: a worn engine caps top-speed, slick
 * tires cut grip, an empty tank stalls the car. Refuel()/Service() restore state.
 *
 * No nodes, no scene access: a Car node owns one keyed by the active vehicle id, calls
 * Drive()/ApplyCrash() as it moves and crashes, reads FuelFraction()/Condition()/TopSpeedFactor()/
 * GripFactor() for the HUD + handling, and persists via ToState()/LoadState(). Stays unit-tested
 * headless (Tests/VehicleConditionTest.cpp).
 *
 * Catalogue entry: {id, tank, economy?, engine_wear?, tire_wear?}. Rows with no/empty id or a
 * non-positive tank are dropped; duplicate ids are dropped.
 *
 * Parity notes:
 * - double throughout; identical literals/tolerances to the Godot oracle.
 * - Godot's Dictionary is insertion-ordered and Ids()/ToState() rely on it; TMap does NOT preserve
 *   order, so an ordered backing store (TArray<FVehicleEntry> + TMap index) mirrors that, like the
 *   sibling FactionStanding/VehicleModShop.
 * - Godot's _is_num (float-or-int) and the to_dict round-trip map to typed fields; LoadState's
 *   "non-numeric values ignored" is preserved via an explicit bHasFuel/etc. per-field flag struct.
 *
 * Deferred Wave-3 adapters (NOT implemented/tested here): the Car component that drives this model,
 * the HUD fuel/condition gauges, the gas-station/mechanic UI loop, and VehicleHandling grip wiring.
 */
class GTC_UE5_API FVehicleCondition
{
public:
    /** Default fuel burn (litres per metre) when an entry omits economy. */
    static constexpr double DefaultEconomy = 0.001;
    /** Wear added per metre driven (before the intensity multiplier). */
    static constexpr double EngineWearPerM = 0.00002;
    static constexpr double TireWearPerM = 0.00003;
    /** Engine wear added by a full-severity (1.0) crash. */
    static constexpr double CrashEngineWear = 0.4;
    /** How much engine vs tire wear weighs into overall condition. */
    static constexpr double EngineConditionWeight = 0.6;
    /** A shot engine still reaches this fraction of top speed; slick tires this much grip. */
    static constexpr double EngineFloor = 0.5;
    static constexpr double TireFloor = 0.6;
    /** Even coasting movement costs at least this intensity (no free driving at intensity 0). */
    static constexpr double MinIntensity = 0.25;

    /** One catalogue row used to construct a roster. */
    struct FVehicleDef
    {
        FString Id;
        double Tank = 0.0;
        double Economy = DefaultEconomy;
        double EngineWear = 0.0;
        double TireWear = 0.0;
    };

    /** Outcome of a Drive() call (mirrors the Godot result Dictionary). */
    struct FDriveResult
    {
        double FuelUsed = 0.0;
        bool bStalled = false;
        double EngineWear = 0.0;
        double TireWear = 0.0;
    };

    /** A persisted per-vehicle row for ToState()/LoadState(). */
    struct FVehicleSave
    {
        FString Id;
        double Fuel = 0.0;
        double EngineWear = 0.0;
        double TireWear = 0.0;

        // LoadState ignores fields not present / non-numeric; these flags mark which to apply.
        bool bHasFuel = true;
        bool bHasEngineWear = true;
        bool bHasTireWear = true;
    };

    /** Construct from a roster; an empty list uses DefaultVehicles(). */
    explicit FVehicleCondition(const TArray<FVehicleDef>& Vehicles = TArray<FVehicleDef>());

    /** Built-in roster (ids deliberately match FChopShop class ids so Condition() feeds Value()). */
    static TArray<FVehicleDef> DefaultVehicles();

    int32 VehicleCount() const;

    bool HasVehicle(const FString& Id) const;

    /** Registered ids in first-seen order. */
    TArray<FString> Ids() const;

    /** Litres of fuel (0 for an unknown vehicle). */
    double FuelOf(const FString& Id) const;

    /** Fuel as a 0..1 fraction of tank capacity. */
    double FuelFraction(const FString& Id) const;

    double EngineWearOf(const FString& Id) const;

    double TireWearOf(const FString& Id) const;

    /** Overall 0..1 condition (1 pristine) blending engine and tire wear. 1.0 for an unknown. */
    double Condition(const FString& Id) const;

    bool IsOutOfFuel(const FString& Id) const;

    /** Top-speed multiplier in [EngineFloor, 1.0]; 0.0 if out of fuel; 1.0 for an unknown. */
    double TopSpeedFactor(const FString& Id) const;

    /** Grip multiplier in [TireFloor, 1.0] from tire wear; 1.0 for an unknown. */
    double GripFactor(const FString& Id) const;

    /** Drive Distance metres at Intensity (1.0 normal). No-op for an unknown / non-positive distance. */
    FDriveResult Drive(const FString& Id, double DistanceM, double Intensity = 1.0);

    /** Add engine wear from a collision (severity 0..1). No-op unknown / non-positive. */
    void ApplyCrash(const FString& Id, double Severity);

    /** Add fuel up to the tank; Liters < 0 fills to full. Returns litres actually added (0 unknown). */
    double Refuel(const FString& Id, double Liters = -1.0);

    /** Mechanic visit: reset the chosen wear channels to 0. No-op for an unknown vehicle. */
    void Service(const FString& Id, bool bEngine = true, bool bTires = true);

    /** Snapshot fuel/wear for the save system, in insertion order (tank/economy are not saved). */
    TArray<FVehicleSave> ToState() const;

    /** Restore fuel/wear for known ids; clamped to valid ranges. Unknown ids / unset fields ignored. */
    void LoadState(const TArray<FVehicleSave>& Data);

private:
    struct FVehicleEntry
    {
        FString Id;
        double Tank = 0.0;
        double Economy = DefaultEconomy;
        double Fuel = 0.0;
        double EngineWear = 0.0;
        double TireWear = 0.0;
    };

    /** Insertion-ordered storage. */
    TArray<FVehicleEntry> Entries;
    /** Id -> index into Entries. */
    TMap<FString, int32> Index;

    void Register(const FVehicleDef& Def);

    const FVehicleEntry* Find(const FString& Id) const;
    FVehicleEntry* Find(const FString& Id);
};
