// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "../NPC/Steering/PedestrianTraffic.h"
#include "../World/RoadNetwork/RoadNetwork.h"
#include "../World/RoadNetwork/CityGrid.h"
#include "../AI/Traffic/TrafficAgent.h"
#include "GTCTrafficSubsystem.generated.h"

class AGTCTrafficVehicle;

/**
 * UGTCTrafficSubsystem — the cars. It streams moving traffic around the player on
 * a REAL, routable road graph (FRoadNetwork built from FCityGrid) and drives each
 * car with FTrafficAgent: the car follows an A* route toward a destination,
 * TURNS at junctions (FTurnChoice), queues behind the car ahead on its lane
 * (FTrafficModel IDM), and picks a fresh destination when it arrives. This
 * replaces the previous version, which slid cars along an invisible axis-aligned
 * grid that never turned a corner.
 *
 * It is still the source the crowd's "look before you cross" reflex reads:
 * UGTCCrowdSubsystem::GatherNearbyCars forwards to QueryCars here, so the
 * pedestrian dodge/curb math (FPedestrianTraffic, wired through FNpcLocomotion)
 * acts on actual traffic. External vehicles (a future player car, scripted
 * convoys) can register so pedestrians react to them too.
 *
 * Frames: the road graph runs in METRES on its own planar frame (the core's "XZ",
 * mapped to UE's XY ground plane); the subsystem scales x100 to centimetres and
 * lifts to the road height when it places the AGTCTrafficVehicle each frame and
 * when it answers QueryCars in UE world space. The grid recenters (rebuild +
 * re-snap cars) only when the player nears its edge, so it costs nothing most
 * frames. Driving state lives in the subsystem; the actor is just the avatar.
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
    int32 TargetVehicles = 28;

    /** Street grid spacing (cm) — distance between parallel streets (one block). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double BlockSize = 6000.0;

    /** Lane offset (cm) to the driver's right of a street centreline (right-hand traffic). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double LaneOffset = 250.0;

    /** Cars are streamed within this radius (cm) of the player and recycled past it. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double StreamRadius = 13000.0;

    /** Desired free-road speed range (cm/s). ~25–50 km/h city traffic. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double DesiredSpeedMin = 700.0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    double DesiredSpeedMax = 1400.0;

    /** Height (cm) above the player's Z that cars drive at. */
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

    /** Cars within Radius of From, as planar pos+vel (UE world cm) for the pedestrian reflex. */
    void QueryCars(const FVector& From, double Radius, TArray<FPedestrianTraffic::FCar>& OutCars) const;

    /** Register / forget an externally-driven vehicle (player car, convoy, ...). */
    void RegisterExternalVehicle(AActor* Vehicle);
    void UnregisterExternalVehicle(AActor* Vehicle);

    /**
     * Spawn a car that drives a FIXED route from StartCm to DestCm and then leaves —
     * the seam a citizen uses to "drive home". Snaps both ends onto the road graph,
     * A*-routes between them, and drives it with the same FTrafficAgent machinery as
     * ambient traffic. When it arrives (route exhausted) OR drives out of the stream
     * window off-screen, OnArrived fires once and the car despawns — so the citizen
     * who got in can surface at the destination (or despawn into the census as "home").
     * Returns false if there's no road graph yet or either end won't snap to a road.
     */
    bool SpawnDirectedCar(const FVector& StartCm, const FVector& DestCm, TFunction<void()> OnArrived);

    /**
     * Snap a world point to the nearest point on the road graph (the curb a parked car
     * would sit at), in UE world cm at the query's height. False if there's no graph
     * yet or the point projects nowhere. Lets a citizen find where to walk to drive off.
     */
    bool NearestRoadPointCm(const FVector& FromCm, FVector& OutCm);

    UFUNCTION(BlueprintCallable, Category = "GTC|Traffic")
    int32 GetLiveVehicleCount() const { return Cars.Num(); }

private:
    /** One streamed car: its network-driving brain plus its on-screen avatar. */
    struct FCar
    {
        FTrafficAgent Agent;
        TWeakObjectPtr<AGTCTrafficVehicle> Actor;
        double HalfLengthM = 2.3; // half body length (m), for bumper-gap math
        /** A directed car drives one fixed route then leaves (a citizen driving home),
         *  instead of touring the city forever like ambient traffic. */
        bool bDirected = false;
        /** Fired once when a directed car reaches its destination (or leaves the
         *  window off-screen). Empty for ambient cars. */
        TFunction<void()> OnArrived;
    };

    APawn* GetPlayerPawn() const;

    /** cm world position -> the road graph's planar metres frame (UE XY -> core XZ). */
    FVector ToNetwork(const FVector& WorldCm) const;
    /** A network pose (planar metres) -> UE world cm at the given road height. */
    FVector ToWorld(const FVector& NetworkM, double WorldZCm) const;

    /** (Re)build the road graph centred on the player and re-snap live cars onto it. */
    void RebuildNetwork(const FVector& PlayerCm);
    /** True when the player has wandered close enough to the grid edge to recenter. */
    bool NeedsRecenter(const FVector& PlayerCm) const;

    void StreamTraffic(const FVector& PlayerCm);
    /** Spawn one car on a random lane within the stream window; false if none found. */
    bool SpawnCar(const FVector& PlayerCm, FCar& OutCar);
    /** A* a fresh route from the car's next junction to a random far node. */
    void AssignRandomRoute(FCar& Car);
    /** Bumper gap (m) and speed (m/s) of the nearest car ahead on the same lane. */
    void NearestLeader(int32 SelfIndex, double& OutGapM, double& OutLeaderSpeedM) const;
    void PlaceActor(const FCar& Car, double RoadZCm) const;
    /** A random node index whose position is within Radius (cm) of the player, or -1. */
    int32 RandomNodeNear(const FVector& PlayerCm, double RadiusCm) const;

    /** The live road graph (metres) and the grid spec that built it. */
    FRoadNetwork RoadNet;
    FCityGrid::FSpec GridSpec;
    FVector GridOriginM = FVector::ZeroVector;
    bool bNetworkBuilt = false;

    TArray<FCar> Cars;
    TArray<TWeakObjectPtr<AActor>> ExternalVehicles;

    double StreamAccum = 0.0;

    static constexpr double StreamIntervalSec = 0.5;
    static constexpr double MetresPerCm = 0.01;
    static constexpr double CmPerMetre = 100.0;
};
