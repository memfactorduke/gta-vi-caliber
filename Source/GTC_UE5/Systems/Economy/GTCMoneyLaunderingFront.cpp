// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCMoneyLaunderingFront.h"
#include "Components/SceneComponent.h"

AGTCMoneyLaunderingFront::AGTCMoneyLaunderingFront()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGTCMoneyLaunderingFront::BeginPlay()
{
    Super::BeginPlay();
    ApplyTuning();
}

void AGTCMoneyLaunderingFront::ApplyTuning()
{
    FMoneyLaundering::FParams Params;
    Params.Cut = static_cast<double>(Cut);
    Params.ThroughputPerHour = static_cast<double>(ThroughputPerHour);
    Params.Capacity = static_cast<double>(Capacity);
    Model.Configure(Params);
}

float AGTCMoneyLaunderingFront::Deposit(float DirtyAmount)
{
    return static_cast<float>(Model.Deposit(static_cast<double>(DirtyAmount)));
}

float AGTCMoneyLaunderingFront::AdvanceHours(float InGameHours)
{
    const double Clean = Model.Advance(static_cast<double>(InGameHours));
    if (Clean > 0.0)
    {
        OnCleanCashProduced.Broadcast(static_cast<float>(Clean));
    }
    return static_cast<float>(Clean);
}

void AGTCMoneyLaunderingFront::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (GameSecondsPerHour <= 0.0f || Model.IsIdle())
    {
        return;
    }
    AdvanceHours(DeltaSeconds / GameSecondsPerHour);
}
