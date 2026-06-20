// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTrafficSubsystem.h"

#include "GTCTrafficVehicle.h"

#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

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
    Cars.Reset();
    ExternalVehicles.Reset();
    bNetworkBuilt = false;
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

// --- Frame conversion -------------------------------------------------------
// The road graph runs in metres on the core's planar "XZ" frame; UE world is
// centimetres on the XY ground plane (Z up). Map UE XY -> core XZ, scale x100.

FVector UGTCTrafficSubsystem::ToNetwork(const FVector& WorldCm) const
{
    return FVector(WorldCm.X * MetresPerCm, 0.0, WorldCm.Y * MetresPerCm);
}

FVector UGTCTrafficSubsystem::ToWorld(const FVector& NetworkM, double WorldZCm) const
{
    return FVector(NetworkM.X * CmPerMetre, NetworkM.Z * CmPerMetre, WorldZCm);
}

// --- Road graph lifecycle ---------------------------------------------------

bool UGTCTrafficSubsystem::NeedsRecenter(const FVector& PlayerCm) const
{
    if (!bNetworkBuilt)
    {
        return true;
    }
    const FVector P = ToNetwork(PlayerCm);
    const double HalfSpanM = 0.5 * GridSpec.BlocksX * GridSpec.BlockSize;
    const double StreamM = StreamRadius * MetresPerCm;
    // Recenter once the stream window no longer comfortably fits inside the grid.
    const double SlackM = HalfSpanM - StreamM - GridSpec.BlockSize;
    const double DistX = FMath::Abs(P.X - GridOriginM.X);
    const double DistZ = FMath::Abs(P.Z - GridOriginM.Z);
    return FMath::Max(DistX, DistZ) > FMath::Max(0.0, SlackM);
}

void UGTCTrafficSubsystem::RebuildNetwork(const FVector& PlayerCm)
{
    const double BlockM = FMath::Max(1.0, BlockSize * MetresPerCm);
    const double StreamM = StreamRadius * MetresPerCm;

    // Size the grid so the stream window plus a two-block margin fits inside it.
    const double HalfNeededM = StreamM + 2.0 * BlockM;
    const int32 BlocksPerSide = FMath::Max(2, FMath::CeilToInt32(HalfNeededM / BlockM));

    // Snap the grid centre to a world-anchored block boundary near the player, so
    // the metres frame is absolute (car positions stay comparable across rebuilds).
    FCityGrid::FSpec Anchor;
    Anchor.Origin = FVector::ZeroVector;
    Anchor.BlockSize = BlockM;
    const FVector SnappedOrigin = FCityGrid::SnapOriginTo(Anchor, ToNetwork(PlayerCm));

    GridSpec.Origin = SnappedOrigin;
    GridSpec.BlockSize = BlockM;
    GridSpec.BlocksX = 2 * BlocksPerSide;
    GridSpec.BlocksZ = 2 * BlocksPerSide;

    RoadNet = FRoadNetwork(1.0);
    for (const TArray<FVector>& Poly : FCityGrid::Polylines(GridSpec))
    {
        RoadNet.AddPolyline(Poly);
    }
    RoadNet.BuildSpatialIndex();
    GridOriginM = SnappedOrigin;
    bNetworkBuilt = true;

    // Re-seed live cars onto the rebuilt graph at their nearest lane point; the old
    // segment indices are gone. Cars that no longer project onto any road are left
    // un-driving and the streamer recycles them.
    for (FCar& Car : Cars)
    {
        const FVector CarPosM = Car.Agent.Pose().Pos;
        const FRoadNetwork::FNearestPoint Hit = RoadNet.NearestPoint(CarPosM);
        if (Hit.bValid)
        {
            Car.Agent.StartOnSegment(RoadNet, Hit.Seg, Hit.Offset, Car.Agent.GetSpeed());
            AssignRandomRoute(Car);
        }
        else
        {
            Car.Agent.StartOnSegment(RoadNet, -1, 0.0, 0.0); // mark un-driving
        }
    }
}

// --- Spawning / routing -----------------------------------------------------

int32 UGTCTrafficSubsystem::RandomNodeNear(const FVector& PlayerCm, double RadiusCm) const
{
    const int32 N = RoadNet.NodeCount();
    if (N == 0)
    {
        return -1;
    }
    const FVector P = ToNetwork(PlayerCm);
    const double RadiusM = RadiusCm * MetresPerCm;
    const double RadiusMSq = RadiusM * RadiusM;
    for (int32 Try = 0; Try < 24; ++Try)
    {
        const int32 Idx = FMath::RandHelper(N);
        const FVector Q = RoadNet.NodePosition(Idx);
        const double DistSq = FMath::Square(Q.X - P.X) + FMath::Square(Q.Z - P.Z);
        if (DistSq <= RadiusMSq)
        {
            return Idx;
        }
    }
    return -1;
}

void UGTCTrafficSubsystem::AssignRandomRoute(FCar& Car)
{
    if (!Car.Agent.IsDriving() || RoadNet.NodeCount() == 0)
    {
        return;
    }
    const int32 FromNode = RoadNet.SegmentEndNode(Car.Agent.Segment());
    if (FromNode < 0)
    {
        return;
    }
    const int32 Goal = FMath::RandHelper(RoadNet.NodeCount());
    Car.Agent.SetRoute(RoadNet.FindPath(FromNode, Goal));
}

bool UGTCTrafficSubsystem::SpawnCar(const FVector& PlayerCm, FCar& OutCar)
{
    UWorld* World = GetWorld();
    if (!World || !bNetworkBuilt)
    {
        return false;
    }

    const int32 StartNode = RandomNodeNear(PlayerCm, StreamRadius);
    if (StartNode < 0)
    {
        return false;
    }
    const TArray<int32>& OutSegs = RoadNet.SegmentsFrom(StartNode);
    if (OutSegs.Num() == 0)
    {
        return false;
    }
    const int32 StartSeg = OutSegs[FMath::RandHelper(OutSegs.Num())];

    OutCar = FCar();
    OutCar.Agent.LateralOffset = LaneOffset * MetresPerCm;
    OutCar.Agent.Drive.DesiredSpeed = FMath::RandRange(DesiredSpeedMin, DesiredSpeedMax) * MetresPerCm;
    if (!OutCar.Agent.StartOnSegment(RoadNet, StartSeg, 0.0, OutCar.Agent.Drive.DesiredSpeed * 0.6))
    {
        return false;
    }
    AssignRandomRoute(OutCar);

    const double RoadZCm = PlayerCm.Z + RoadZOffset;
    const FLanePath::FPose Pose = OutCar.Agent.Pose();
    const FVector SpawnPos = ToWorld(Pose.Pos, RoadZCm);
    const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Pose.Heading.Z, Pose.Heading.X));
    const FTransform Xform(FRotator(0.0, Yaw, 0.0), SpawnPos);

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
    OutCar.Actor = Car;
    OutCar.HalfLengthM = FMath::Max(0.5, Car->BodyHalfExtent.X * MetresPerCm);
    return true;
}

void UGTCTrafficSubsystem::NearestLeader(int32 SelfIndex, double& OutGapM, double& OutLeaderSpeedM) const
{
    OutGapM = FTrafficAgent::OpenRoadGap;
    OutLeaderSpeedM = 0.0;
    const FCar& Self = Cars[SelfIndex];
    if (!Self.Agent.IsDriving())
    {
        return;
    }
    const int32 SelfSeg = Self.Agent.Segment();
    const double SelfS = Self.Agent.Arc();

    for (int32 i = 0; i < Cars.Num(); ++i)
    {
        if (i == SelfIndex)
        {
            continue;
        }
        const FCar& Other = Cars[i];
        // Same directed lane only: cars queue behind the one ahead on their segment.
        if (!Other.Agent.IsDriving() || Other.Agent.Segment() != SelfSeg)
        {
            continue;
        }
        const double Ahead = Other.Agent.Arc() - SelfS;
        if (Ahead <= 0.0)
        {
            continue;
        }
        const double Gap = Ahead - (Self.HalfLengthM + Other.HalfLengthM);
        if (Gap < OutGapM)
        {
            OutGapM = Gap;
            OutLeaderSpeedM = Other.Agent.GetSpeed();
        }
    }
}

void UGTCTrafficSubsystem::PlaceActor(const FCar& Car, double RoadZCm) const
{
    AGTCTrafficVehicle* Actor = Car.Actor.Get();
    if (!Actor)
    {
        return;
    }
    const FLanePath::FPose Pose = Car.Agent.Pose();
    const FVector WorldPos = ToWorld(Pose.Pos, RoadZCm);
    const double Yaw = FMath::RadiansToDegrees(FMath::Atan2(Pose.Heading.Z, Pose.Heading.X));
    Actor->SetActorLocationAndRotation(WorldPos, FRotator(0.0, Yaw, 0.0), /*bSweep*/ false);
}

void UGTCTrafficSubsystem::StreamTraffic(const FVector& PlayerCm)
{
    const FVector PlayerM = ToNetwork(PlayerCm);
    const double KeepM = StreamRadius * 1.15 * MetresPerCm;
    const double KeepMSq = KeepM * KeepM;

    // Recycle cars that wandered out of the window, lost their actor, or stranded.
    for (int32 i = Cars.Num() - 1; i >= 0; --i)
    {
        const FCar& Car = Cars[i];
        const FVector CarM = Car.Agent.Pose().Pos;
        const double DistSq = FMath::Square(CarM.X - PlayerM.X) + FMath::Square(CarM.Z - PlayerM.Z);
        if (!Car.Actor.IsValid() || !Car.Agent.IsDriving() || DistSq > KeepMSq)
        {
            if (AGTCTrafficVehicle* Actor = Car.Actor.Get())
            {
                Actor->Destroy();
            }
            Cars.RemoveAt(i);
        }
    }

    int32 Guard = 0;
    while (Cars.Num() < TargetVehicles && Guard++ < TargetVehicles)
    {
        FCar Car;
        if (!SpawnCar(PlayerCm, Car))
        {
            break;
        }
        Cars.Add(Car);
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
    const FVector PlayerCm = Player->GetActorLocation();

    if (NeedsRecenter(PlayerCm))
    {
        RebuildNetwork(PlayerCm);
    }

    StreamAccum += DeltaTime;
    if (bStreamTraffic && StreamAccum >= StreamIntervalSec)
    {
        StreamAccum = 0.0;
        StreamTraffic(PlayerCm);
    }

    const double Dt = static_cast<double>(DeltaTime);
    const double RoadZCm = PlayerCm.Z + RoadZOffset;
    for (int32 i = 0; i < Cars.Num(); ++i)
    {
        FCar& Car = Cars[i];
        if (!Car.Agent.IsDriving())
        {
            continue; // recycled next stream pass
        }
        // Arrived at its destination? Pick a fresh one so it keeps touring the city.
        if (Car.Agent.IsRouteExhausted())
        {
            AssignRandomRoute(Car);
        }

        double GapM = FTrafficAgent::OpenRoadGap;
        double LeaderSpeedM = 0.0;
        NearestLeader(i, GapM, LeaderSpeedM);

        Car.Agent.Step(RoadNet, Dt, GapM, LeaderSpeedM);
        PlaceActor(Car, RoadZCm);
    }
}

// --- Queries ----------------------------------------------------------------

void UGTCTrafficSubsystem::QueryCars(
    const FVector& From, double Radius, TArray<FPedestrianTraffic::FCar>& OutCars) const
{
    OutCars.Reset();
    const double RadiusSq = Radius * Radius;
    const double RoadZCm = From.Z; // the reflex is planar; height is not used.

    for (const FCar& Car : Cars)
    {
        if (!Car.Agent.IsDriving())
        {
            continue;
        }
        const FLanePath::FPose Pose = Car.Agent.Pose();
        const FVector WorldPos = ToWorld(Pose.Pos, RoadZCm);
        if (FVector::DistSquaredXY(WorldPos, From) > RadiusSq)
        {
            continue;
        }
        // Velocity in UE world cm/s: heading (core XZ) -> UE XY, scaled by speed.
        const double SpeedCm = Car.Agent.GetSpeed() * CmPerMetre;
        FPedestrianTraffic::FCar Out;
        Out.Pos = WorldPos;
        Out.Vel = FVector(Pose.Heading.X * SpeedCm, Pose.Heading.Z * SpeedCm, 0.0);
        OutCars.Add(Out);
    }

    // External (player car, scripted) vehicles react too.
    for (const TWeakObjectPtr<AActor>& Weak : ExternalVehicles)
    {
        const AActor* V = Weak.Get();
        if (!V || FVector::DistSquaredXY(V->GetActorLocation(), From) > RadiusSq)
        {
            continue;
        }
        FPedestrianTraffic::FCar Out;
        Out.Pos = V->GetActorLocation();
        Out.Vel = V->GetVelocity();
        OutCars.Add(Out);
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
