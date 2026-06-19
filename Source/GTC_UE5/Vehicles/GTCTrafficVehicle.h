// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GTCTrafficVehicle.generated.h"

class UBoxComponent;
class UStaticMeshComponent;

/**
 * AGTCTrafficVehicle — a piece of moving traffic. It is deliberately dumb: the
 * UGTCTrafficSubsystem owns the driving logic (lane, speed, IDM car-following)
 * and just places this actor each frame. The actor exists to be SEEN and to give
 * pedestrians something physical on the road, which is what closes the loop on
 * "look before you cross": the crowd's traffic reflex reads these cars' positions
 * and velocities and steers pedestrians clear.
 *
 * A box collider is the root so it has a car-sized footprint with collision even
 * before a mesh is assigned; a StaticMesh child renders it (assign a real car
 * mesh on a BP subclass — set VehicleMesh — for production visuals).
 */
UCLASS()
class GTC_UE5_API AGTCTrafficVehicle : public AActor
{
    GENERATED_BODY()

public:
    AGTCTrafficVehicle();

    /** Half-extents (cm) of the car body — also the footprint the subsystem uses
     *  for bumper-to-bumper gap math. ~ a 4.6 m long, 1.9 m wide sedan. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Traffic")
    FVector BodyHalfExtent = FVector(230.0, 95.0, 75.0);

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Traffic")
    TObjectPtr<UBoxComponent> Body;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "GTC|Traffic")
    TObjectPtr<UStaticMeshComponent> VehicleMesh;
};
