// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCPlaceMarker.h"

#include "GTCPlaceRegistrySubsystem.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"

AGTCPlaceMarker::AGTCPlaceMarker()
{
    PrimaryActorTick.bCanEverTick = false;
    // A bare scene root gives the marker a movable transform in the editor; it has
    // no runtime cost and nothing to render (designers see the actor gizmo).
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGTCPlaceMarker::BeginPlay()
{
    Super::BeginPlay();
    if (UWorld* World = GetWorld())
    {
        if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
        {
            Handle = Places->RegisterPlace(Kind, GetActorLocation(), Capacity);
        }
    }
}

void AGTCPlaceMarker::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (Handle != 0)
    {
        if (UWorld* World = GetWorld())
        {
            if (UGTCPlaceRegistrySubsystem* Places = World->GetSubsystem<UGTCPlaceRegistrySubsystem>())
            {
                Places->UnregisterPlace(Handle);
            }
        }
        Handle = 0;
    }
    Super::EndPlay(EndPlayReason);
}
