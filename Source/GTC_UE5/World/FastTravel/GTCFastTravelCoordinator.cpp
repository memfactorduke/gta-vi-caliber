// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCFastTravelCoordinator.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"

#include "GTCFastTravelHub.h"
#include "../../Systems/Wanted/WantedSubsystem.h"

AGTCFastTravelCoordinator::AGTCFastTravelCoordinator()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AGTCFastTravelCoordinator::BeginPlay()
{
    Super::BeginPlay();
    RebuildNetwork();
}

void AGTCFastTravelCoordinator::RebuildNetwork()
{
    Hubs.Reset();
    Network.Reset();
    UWorld* World = GetWorld();
    if (World == nullptr)
    {
        return;
    }

    TArray<AActor*> Found;
    UGameplayStatics::GetAllActorsWithTag(World, AGTCFastTravelHub::HubTag, Found);
    for (AActor* A : Found)
    {
        if (AGTCFastTravelHub* Hub = Cast<AGTCFastTravelHub>(A))
        {
            FFastTravelNetwork::FHub Entry;
            Entry.Location = Hub->GetActorLocation();
            Entry.Kind = Hub->GetHubKind();
            Entry.Id = Hub->GetHubId();
            Entry.bUnlocked = Hub->IsUnlocked();
            Network.Add(Entry);
            Hubs.Add(Hub);
        }
    }
}

int32 AGTCFastTravelCoordinator::ReadStars() const
{
    if (const UGameInstance* GI = GetGameInstance())
    {
        if (const UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
        {
            return Wanted->Stars();
        }
    }
    return 0;
}

APawn* AGTCFastTravelCoordinator::PlayerPawn() const
{
    if (const UWorld* World = GetWorld())
    {
        if (const APlayerController* PC = World->GetFirstPlayerController())
        {
            return PC->GetPawn();
        }
    }
    return nullptr;
}

FVector AGTCFastTravelCoordinator::PlayerLocation() const
{
    const APawn* Pawn = PlayerPawn();
    return Pawn != nullptr ? Pawn->GetActorLocation() : FVector::ZeroVector;
}

bool AGTCFastTravelCoordinator::CanDepartNow() const
{
    // No threat tracking yet -> pass a large distance so only the wanted gate applies.
    return FFastTravelNetwork::CanDepart(ReadStars() > 0, 1.0e9, ThreatSafeRangeM);
}

int32 AGTCFastTravelCoordinator::QuoteFareToIndex(int32 HubIndex) const
{
    if (!Network.IsValidIndex(HubIndex))
    {
        return 0;
    }
    const int32 From = FFastTravelNetwork::NearestHub(Network, PlayerLocation(), 0.0);
    const FFastTravelNetwork::FHub& FromHub =
        Network.IsValidIndex(From) ? Network[From] : Network[HubIndex];
    return FFastTravelNetwork::Fare(FromHub, Network[HubIndex], FarePerKm, BaseFare);
}

void AGTCFastTravelCoordinator::RelocatePlayerTo(const FVector& Dest)
{
    APawn* Pawn = PlayerPawn();
    if (Pawn == nullptr)
    {
        return;
    }

    UWorld* World = GetWorld();
    APlayerController* PC = World != nullptr ? World->GetFirstPlayerController() : nullptr;
    if (PC != nullptr && PC->PlayerCameraManager != nullptr && FadeSeconds > 0.0f)
    {
        // Quick fade from black on arrival as a low-cost transition.
        PC->PlayerCameraManager->StartCameraFade(1.0f, 0.0f, FadeSeconds, FColor::Black);
    }
    Pawn->SetActorLocation(Dest + FVector(0.0, 0.0, ArriveZOffsetCm), /*bSweep*/ false);
}

bool AGTCFastTravelCoordinator::RequestHopToIndex(int32 HubIndex)
{
    if (!Network.IsValidIndex(HubIndex) || !CanDepartNow())
    {
        return false;
    }
    RelocatePlayerTo(Network[HubIndex].Location);
    return true;
}

bool AGTCFastTravelCoordinator::RequestHopToNearestOfKind(EHubKind Kind)
{
    if (!CanDepartNow())
    {
        return false;
    }
    const int32 Index = FFastTravelNetwork::NearestHubOfKind(Network, PlayerLocation(), Kind, 0.0);
    if (!Network.IsValidIndex(Index))
    {
        return false;
    }
    RelocatePlayerTo(Network[Index].Location);
    return true;
}
