// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTrafficSubsystem.h"

#include "GTCTrafficVehicle.h"
#include "../Worldcore/TrafficModel.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

namespace
{
    // IDM parameters (metres / seconds), comfortable city driving.
    constexpr double IdmMaxAccel = 1.5;     // a
    constexpr double IdmComfortDecel = 2.0; // b
    constexpr double IdmMinGap = 2.0;       // s0 (m)
    constexpr double IdmTimeHeadway = 1.4;  // T (s)
    constexpr double BigGap = 1.0e6;        // cm — "open road"
}

void UGTCTrafficSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (!VehicleClass)
    {
        VehicleClass = AGTCTrafficVehicle::StaticClass();
    }
}

void UGTCTrafficSubsystem::Deinitialize()
{
    Agents.Reset();
    ExternalVehicles.Reset();
    Super::Deinitialize();
}

bool UGTCTrafficSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
    return WorldType == EWorldType::Game || WorldType == EWorldType::PIE;
}

TStatId UGTCTrafficSubsystem::GetStatId() const
{
    RETURN_QUICK_DECLARE_CYCLE_STAT(UGTCTrafficSubsystem, STATGROUP_Tickables);
}

APawn* UGTCTrafficSubsystem::GetPlayerPawn() const
{
    if (const UWorld* World = GetWorld())
    {
        if (APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

void UGTCTrafficSubsystem::NearestLeader(int32 SelfIndex, double& OutGap, double& OutLeaderSpeed) const
{
    OutGap = BigGap;
    OutLeaderSpeed = 0.0;
    const FTrafficAgent& Self = Agents[SelfIndex];
    const double SelfAlong = (Self.Axis == 0 ? Self.Pos.X : Self.Pos.Y) * Self.Dir;

    for (int32 i = 0; i < Agents.Num(); ++i)
    {
        if (i == SelfIndex)
        {
            continue;
        }
        const FTrafficAgent& Other = Agents[i];
        if (Other.Axis != Self.Axis || Other.Dir != Self.Dir)
        {
            continue;
        }
        if (FMath::Abs(Other.LaneCoord - Self.LaneCoord) > LaneMatchTol)
        {
            continue;
        }
        const double OtherAlong = (Other.Axis == 0 ? Other.Pos.X : Other.Pos.Y) * Other.Dir;
        const double Ahead = OtherAlong - SelfAlong; // >0 means in front (along travel)
        if (Ahead <= 0.0)
        {
            continue;
        }
        const double Gap = Ahead - (Self.HalfLength + Other.HalfLength);
        if (Gap < OutGap)
        {
            OutGap = Gap;
            OutLeaderSpeed = Other.Speed;
        }
    }
}

bool UGTCTrafficSubsystem::SpawnAgent(const FVector& Around, FTrafficAgent& OutAgent) const
{
    UWorld* World = GetWorld();
    if (!World)
    {
        return false;
    }

    // Pick a street: an axis, a street line snapped to the block grid near the
    // player, a direction, and the right-hand lane for that direction.
    const int32 Axis = FMath::RandBool() ? 0 : 1;
    const int32 Dir = FMath::RandBool() ? 1 : -1;

    const double AroundPerp = (Axis == 0 ? Around.Y : Around.X);
    // Choose one of the few streets straddling the player on the perpendicular axis.
    const int32 StreetIdx = FMath::RandRange(-2, 2);
    const double StreetLine = FMath::GridSnap(AroundPerp, BlockSize) + StreetIdx * BlockSize;
    // Right-hand lane: offset to the correct side of the centreline for Dir.
    const double LaneCoord = StreetLine + (Dir > 0 ? LaneOffset : -LaneOffset);

    // Enter from the up-stream edge of the stream window so the car drives across it.
    const double AroundAlong = (Axis == 0 ? Around.X : Around.Y);
    const double StartAlong = AroundAlong - Dir * StreamRadius;

    OutAgent = FTrafficAgent();
    OutAgent.Axis = Axis;
    OutAgent.Dir = Dir;
    OutAgent.LaneCoord = LaneCoord;
    OutAgent.DesiredSpeed = FMath::RandRange(DesiredSpeedMin, DesiredSpeedMax);
    OutAgent.Speed = OutAgent.DesiredSpeed * 0.6;

    const double Z = Around.Z + RoadZOffset;
    OutAgent.Pos = (Axis == 0) ? FVector(StartAlong, LaneCoord, Z) : FVector(LaneCoord, StartAlong, Z);

    const FRotator Rot(0.0f, (Axis == 0 ? (Dir > 0 ? 0.0f : 180.0f) : (Dir > 0 ? 90.0f : -90.0f)), 0.0f);
    const FTransform Xform(Rot, OutAgent.Pos);

    TSubclassOf<AGTCTrafficVehicle> SpawnClass = VehicleClass;
    if (!SpawnClass)
    {
        SpawnClass = AGTCTrafficVehicle::StaticClass();
    }
    AGTCTrafficVehicle* Car = World->SpawnActorDeferred<AGTCTrafficVehicle>(
        SpawnClass, Xform, nullptr, nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if (!Car)
    {
        return false;
    }
    Car->FinishSpawning(Xform);
    OutAgent.Actor = Car;
    OutAgent.HalfLength = Car->BodyHalfExtent.X;
    return true;
}

void UGTCTrafficSubsystem::PlaceActor(const FTrafficAgent& Agent) const
{
    AGTCTrafficVehicle* Car = Agent.Actor.Get();
    if (!Car)
    {
        return;
    }
    const FRotator Rot(
        0.0f, (Agent.Axis == 0 ? (Agent.Dir > 0 ? 0.0f : 180.0f) : (Agent.Dir > 0 ? 90.0f : -90.0f)), 0.0f);
    Car->SetActorLocationAndRotation(Agent.Pos, Rot, /*bSweep*/ false);
}

void UGTCTrafficSubsystem::StreamTraffic(const FVector& Around)
{
    // Retire cars that wandered out of the window; respawn to hold the target count.
    const double KeepSq = FMath::Square(StreamRadius * 1.15);
    for (int32 i = Agents.Num() - 1; i >= 0; --i)
    {
        const FTrafficAgent& A = Agents[i];
        if (!A.Actor.IsValid() || FVector::DistSquaredXY(A.Pos, Around) > KeepSq)
        {
            if (AGTCTrafficVehicle* Car = A.Actor.Get())
            {
                Car->Destroy();
            }
            Agents.RemoveAt(i);
        }
    }

    int32 Guard = 0;
    while (Agents.Num() < TargetVehicles && Guard++ < TargetVehicles)
    {
        FTrafficAgent Agent;
        if (!SpawnAgent(Around, Agent))
        {
            break;
        }
        PlaceActor(Agent);
        Agents.Add(Agent);
    }
}

void UGTCTrafficSubsystem::Tick(float DeltaTime)
{
    UWorld* World = GetWorld();
    if (!World || !World->IsGameWorld())
    {
        return;
    }

    APawn* Player = GetPlayerPawn();
    if (!Player)
    {
        return; // No anchor to stream traffic around.
    }
    const FVector PlayerPos = Player->GetActorLocation();

    StreamAccum += DeltaTime;
    if (bStreamTraffic && StreamAccum >= StreamIntervalSec)
    {
        StreamAccum = 0.0;
        StreamTraffic(PlayerPos);
    }

    // Drive every car: IDM longitudinal control, then advance along its lane.
    const double Dt = static_cast<double>(DeltaTime);
    for (int32 i = 0; i < Agents.Num(); ++i)
    {
        FTrafficAgent& A = Agents[i];

        double GapCm = BigGap;
        double LeaderSpeedCm = 0.0;
        NearestLeader(i, GapCm, LeaderSpeedCm);

        // IDM works in metres; convert at the boundary.
        const double AccelMs2 = FTrafficModel::CarFollowingAccel(
            A.Speed / 100.0, GapCm / 100.0, LeaderSpeedCm / 100.0, A.DesiredSpeed / 100.0,
            IdmMaxAccel, IdmComfortDecel, IdmMinGap, IdmTimeHeadway);

        A.Speed = FMath::Max(0.0, A.Speed + AccelMs2 * 100.0 * Dt);

        const double Step = A.Dir * A.Speed * Dt;
        if (A.Axis == 0)
        {
            A.Pos.X += Step;
        }
        else
        {
            A.Pos.Y += Step;
        }
        A.Pos.Z = PlayerPos.Z + RoadZOffset;

        PlaceActor(A);
    }
}

void UGTCTrafficSubsystem::QueryCars(
    const FVector& From, double Radius, TArray<FPedestrianTraffic::FCar>& OutCars) const
{
    OutCars.Reset();
    const double RadiusSq = Radius * Radius;

    for (const FTrafficAgent& A : Agents)
    {
        if (FVector::DistSquaredXY(A.Pos, From) > RadiusSq)
        {
            continue;
        }
        FPedestrianTraffic::FCar Car;
        Car.Pos = A.Pos;
        Car.Vel = (A.Axis == 0) ? FVector(A.Dir * A.Speed, 0.0, 0.0) : FVector(0.0, A.Dir * A.Speed, 0.0);
        OutCars.Add(Car);
    }

    // External (player car, scripted) vehicles react too.
    for (const TWeakObjectPtr<AActor>& Weak : ExternalVehicles)
    {
        const AActor* V = Weak.Get();
        if (!V)
        {
            continue;
        }
        if (FVector::DistSquaredXY(V->GetActorLocation(), From) > RadiusSq)
        {
            continue;
        }
        FPedestrianTraffic::FCar Car;
        Car.Pos = V->GetActorLocation();
        Car.Vel = V->GetVelocity();
        OutCars.Add(Car);
    }
}

void UGTCTrafficSubsystem::RegisterExternalVehicle(AActor* Vehicle)
{
    if (Vehicle)
    {
        ExternalVehicles.AddUnique(Vehicle);
    }
}

void UGTCTrafficSubsystem::UnregisterExternalVehicle(AActor* Vehicle)
{
    ExternalVehicles.RemoveAll(
        [Vehicle](const TWeakObjectPtr<AActor>& V) { return !V.IsValid() || V.Get() == Vehicle; });
}
