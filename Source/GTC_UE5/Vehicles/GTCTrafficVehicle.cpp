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
    // ...but do NOT physically block the player. The subsystem moves this body by
    // teleport (SetActorLocationAndRotation, no sweep) every frame; a blocking box
    // teleported onto the player's capsule is depenetrated by the physics solver,
    // which ejects the player along the car's length — the "shoved forward/back when
    // standing on the road" bug. Responding to the Pawn channel with Overlap keeps the
    // footprint queryable (pedestrian reflex, traces, bullets) while removing the shove.
    // Proper follow-up once cars are visible: make traffic brake for the player so cars
    // are solid AND never drive through you, then this can go back to Block.
    Body->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
    Body->SetMobility(EComponentMobility::Movable);
    RootComponent = Body;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);
    VehicleMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    // Mesh asset is assigned on a BP subclass for production visuals; the box
    // gives a valid car footprint meanwhile.
}
