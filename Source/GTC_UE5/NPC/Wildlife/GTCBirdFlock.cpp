// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCBirdFlock.h"
#include "Components/SceneComponent.h"

AGTCBirdFlock::AGTCBirdFlock()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

FBirdFlock::FParams AGTCBirdFlock::MakeParams() const
{
    FBirdFlock::FParams Params;
    Params.BurstSeconds = static_cast<double>(BurstSeconds);
    Params.MinWheelSeconds = static_cast<double>(MinWheelSeconds);
    Params.CalmBeforeSettle = static_cast<double>(CalmBeforeSettle);
    Params.SettleSeconds = static_cast<double>(SettleSeconds);
    return Params;
}

EGtcBirdPhase AGTCBirdFlock::GetPhase() const
{
    switch (Model.Phase())
    {
    case FBirdFlock::EPhase::Startled: return EGtcBirdPhase::Startled;
    case FBirdFlock::EPhase::Wheeling: return EGtcBirdPhase::Wheeling;
    case FBirdFlock::EPhase::Settling: return EGtcBirdPhase::Settling;
    default:                           return EGtcBirdPhase::Perched;
    }
}

float AGTCBirdFlock::GetAltitude() const
{
    return static_cast<float>(Model.Altitude(MakeParams()));
}

void AGTCBirdFlock::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Model.Update(bStartlePending, static_cast<double>(DeltaSeconds), MakeParams());
    bStartlePending = false;

    const EGtcBirdPhase Phase = GetPhase();
    if (Phase != LastPhase)
    {
        LastPhase = Phase;
        OnFlockPhaseChanged.Broadcast(Phase);
    }
}
