// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCCoastGuardDirector.h"

#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Math/UnrealMathUtility.h"

#include "GTCCoastGuardBoat.h"
#include "GTCVehicleMedium.h"
#include "../PoliceDispatch/AirSeaEscalation.h"
#include "../../Systems/Wanted/WantedSubsystem.h"
#include "../../World/Environment/GTCOceanSubsystem.h"

AGTCCoastGuardDirector::AGTCCoastGuardDirector()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AGTCCoastGuardDirector::BeginPlay()
{
    Super::BeginPlay();
    if (!BoatClass)
    {
        BoatClass = AGTCCoastGuardBoat::StaticClass();
    }
}

APawn* AGTCCoastGuardDirector::GetPlayerPawn() const
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

int32 AGTCCoastGuardDirector::ReadStars() const
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

void AGTCCoastGuardDirector::Prune()
{
    Boats.RemoveAll([](const TWeakObjectPtr<AGTCCoastGuardBoat>& B)
    {
        return !B.IsValid() || B->IsDead();
    });
}

void AGTCCoastGuardDirector::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    StreamAccum += DeltaSeconds;
    if (StreamAccum < StreamIntervalSec)
    {
        return;
    }
    StreamAccum = 0.0;
    StreamCoastGuard();
}

void AGTCCoastGuardDirector::StreamCoastGuard()
{
    Prune();

    UWorld* World = GetWorld();
    APawn* Player = GetPlayerPawn();
    if (World == nullptr || Player == nullptr)
    {
        return;
    }

    const int32 Stars = ReadStars();
    const EPlayerMedium Medium = GTCVehicleMedium::MediumOf(Player);
    const int32 Desired = FAirSeaEscalation::CoastGuardBoatCount(Stars, Medium);

    // Stand down when the player is off the water or the heat has cleared.
    if (Desired <= 0)
    {
        for (const TWeakObjectPtr<AGTCCoastGuardBoat>& B : Boats)
        {
            if (B.IsValid())
            {
                B->Destroy();
            }
        }
        Boats.Reset();
        return;
    }

    const UGTCOceanSubsystem* Ocean = World->GetSubsystem<UGTCOceanSubsystem>();
    const FVector PlayerLoc = Player->GetActorLocation();

    // Top up the interceptor count, spawning offshore around the player.
    for (int32 I = Boats.Num(); I < Desired; ++I)
    {
        const double Angle = (UE_DOUBLE_PI * 2.0) * (static_cast<double>(SpawnCounter++) * 0.61803398875); // golden-angle spread
        double X = PlayerLoc.X + FMath::Cos(Angle) * SpawnRadiusCm;
        double Y = PlayerLoc.Y + FMath::Sin(Angle) * SpawnRadiusCm;
        double Z = Ocean != nullptr ? Ocean->HeightAtCm(X, Y) : PlayerLoc.Z;

        const FTransform Xform(FRotator::ZeroRotator, FVector(X, Y, Z));
        AGTCCoastGuardBoat* Boat = World->SpawnActor<AGTCCoastGuardBoat>(BoatClass, Xform);
        if (Boat != nullptr)
        {
            Boats.Add(Boat);
        }
    }
}
