// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "../NPC/Steering/PedestrianTraffic.h"
#include "GTCTrafficSubsystem.generated.h"

class AGTCTrafficVehicle;

/**
 * UGTCTrafficSubsystem — the cars. It streams a grid of moving traffic around the
 * player and drives each car with the project's existing, parity-tested
 * Intelligent Driver Model (FTrafficModel): cars cruise toward a desired speed on
 * open road and brake for the car ahead, so they queue and flow instead of
 * clipping through each other. Lanes are an axis-aligned street grid (a city
 * block layout), which reads as traffic and, more importantly, gives pedestrians
 * real vehicles to watch for.
 *
 * It is the source the crowd's "look before you cross" reflex was waiting on:
 * UGTCCrowdSubsystem::GatherNearbyCars now forwards to QueryCars here, so the
 * pedestrian dodge/curb math (FPedestrianTraffic, already wired through
 * FNpcLocomotion) acts on actual traffic.
 *
 * Driving state lives in the subsystem (FTrafficAgent), not on the actors — the
 * AGTCTrafficVehicle is just placed each frame. External vehicles (a future
 * player car, scripted convoys) can also register so pedestrians react to them.
 * Ticks via FTickableGameObject.
 */
UCLASS()
class GTC_UE5_API UGTCTrafficSubsystem : public UWorldSubsystem, public FTickableGameObject
{
    GENERATED_BODY()

public:
    // --- Tuning -----------------------------------------------------------------

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    bool bStreamTraffic = true;

    /** Class to spawn for streamed cars (BP subclass with a mesh recommended). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    TSubclassOf<AGTCTrafficVehicle> VehicleClass;

    /** Target live car count around the player. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    int32 TargetVehicles = 24;

    /** Street grid spacing (cm) — distance between parallel streets. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double BlockSize = 8000.0;

    /** Lane offset (cm) either side of a street centreline (right-hand traffic). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double LaneOffset = 250.0;

    /** Cars are streamed within this radius (cm) of the player and recycled past it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double StreamRadius = 9000.0;

    /** Desired free-road speed range (cm/s). ~25–50 km/h city traffic. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double DesiredSpeedMin = 700.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double DesiredSpeedMax = 1400.0;

    /** Ground height (cm) cars drive at, plus the player's Z. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double RoadZOffset = 30.0;

    // --- UWorldSubsystem / FTickableGameObject ----------------------------------

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickable() const override { return true; }
    virtual bool IsTickableInEditor() const override { return false; }
    virtual ETickableTickType GetTickableTickType() const override { return ETickableTickType::Conditional; }

    // --- Queries ----------------------------------------------------------------

    /** Cars within Radius of From, as planar pos+vel for the pedestrian reflex. */
    void QueryCars(const FVector& From, double Radius, TArray<FPedestrianTraffic::FCar>& OutCars) const;

    /** Register / forget an externally-driven vehicle (player car, convoy, ...). */
    void RegisterExternalVehicle(AActor* Vehicle);
    void UnregisterExternalVehicle(AActor* Vehicle);

    UFUNCTION(BlueprintCallable, Category = "GTC|Traffic")
    int32 GetLiveVehicleCount() const { return Agents.Num(); }

private:
    /** One streamed car's driving state (the actor is just its avatar). */
    struct FTrafficAgent
    {
        TWeakObjectPtr<AGTCTrafficVehicle> Actor;
        FVector Pos = FVector::ZeroVector;
        double Speed = 0.0;        // cm/s, always >= 0 (direction is in Dir/Axis)
        double DesiredSpeed = 1000.0;
        double HalfLength = 230.0; // along travel, for bumper gap math
        int32 Axis = 0;            // 0 = travels along X, 1 = along Y
        int32 Dir = 1;             // +1 / -1
        double LaneCoord = 0.0;    // fixed perpendicular coordinate (the lane)
    };

    APawn* GetPlayerPawn() const;
    void StreamTraffic(const FVector& Around);
    bool SpawnAgent(const FVector& Around, FTrafficAgent& OutAgent) const;
    /** Bumper gap (cm) and speed of the nearest car ahead in the same lane.
     *  OutGap is BIG and OutLeaderSpeed 0 when the road ahead is clear. */
    void NearestLeader(int32 SelfIndex, double& OutGap, double& OutLeaderSpeed) const;
    void PlaceActor(const FTrafficAgent& Agent) const;

    TArray<FTrafficAgent> Agents;
    TArray<TWeakObjectPtr<AActor>> ExternalVehicles;

    double StreamAccum = 0.0;

    static constexpr double StreamIntervalSec = 0.5;
    static constexpr double LaneMatchTol = 120.0; // cm; same-lane coordinate tolerance
};
