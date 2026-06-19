// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTrafficVehicle.h"

#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"

AGTCTrafficVehicle::AGTCTrafficVehicle()
{
    PrimaryActorTick.bCanEverTick = false; // The subsystem drives placement.

    Body = CreateDefaultSubobject<UBoxComponent>(TEXT("Body"));
    Body->SetBoxExtent(BodyHalfExtent);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Body->SetCollisionObjectType(ECollisionChannel::ECC_WorldDynamic);
    Body->SetCollisionResponseToAllChannels(ECR_Block);
    Body->SetMobility(EComponentMobility::Movable);
    RootComponent = Body;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);
    VehicleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // Mesh asset is assigned on a BP subclass for production visuals; the box
    // gives a valid car footprint meanwhile.
}
