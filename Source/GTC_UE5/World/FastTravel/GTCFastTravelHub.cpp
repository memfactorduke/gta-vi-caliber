// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCFastTravelHub.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "../../Vehicles/Pawn/GTCKinematicVehiclePawn.h"

const FName AGTCFastTravelHub::HubTag(TEXT("fast_travel_hub"));

AGTCFastTravelHub::AGTCFastTravelHub()
{
    PrimaryActorTick.bCanEverTick = false;
    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);
}

void AGTCFastTravelHub::BeginPlay()
{
    Super::BeginPlay();
    Tags.AddUnique(HubTag);
    SpawnParkedVehicle();
}

void AGTCFastTravelHub::SpawnParkedVehicle()
{
    if (!ParkedVehicleClass)
    {
        return;
    }
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }
    const FTransform Xform(GetActorRotation(), GetActorLocation() + GetActorRotation().RotateVector(ParkedVehicleOffset));
    World->SpawnActor<AGTCKinematicVehiclePawn>(ParkedVehicleClass, Xform);
}
