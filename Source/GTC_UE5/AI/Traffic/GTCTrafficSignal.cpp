// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTrafficSignal.h"
#include "Components/SceneComponent.h"

AGTCTrafficSignal::AGTCTrafficSignal()
{
    PrimaryActorTick.bCanEverTick = true;
    RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void AGTCTrafficSignal::BeginPlay()
{
    Super::BeginPlay();
    ConfigureSignal();
}

void AGTCTrafficSignal::ConfigureSignal()
{
    TArray<FTrafficSignal::FPhase> CorePhases;
    CorePhases.Reserve(Phases.Num());
    for (const FGtcSignalPhase& In : Phases)
    {
        FTrafficSignal::FPhase Phase;
        Phase.GoGroups = In.GoGroups;
        Phase.GreenSeconds = static_cast<double>(In.GreenSeconds);
        Phase.YellowSeconds = static_cast<double>(In.YellowSeconds);
        CorePhases.Add(MoveTemp(Phase));
    }
    Model.Configure(MoveTemp(CorePhases), static_cast<double>(AllRedSeconds), static_cast<double>(OffsetSeconds));
    LastActivePhase = Model.ActivePhase();
}

EGtcSignalState AGTCTrafficSignal::StateFor(int32 Group) const
{
    switch (Model.StateFor(Group))
    {
    case FTrafficSignal::ESignal::Green:  return EGtcSignalState::Green;
    case FTrafficSignal::ESignal::Yellow: return EGtcSignalState::Yellow;
    default:                              return EGtcSignalState::Red;
    }
}

void AGTCTrafficSignal::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    Model.Advance(static_cast<double>(DeltaSeconds));

    const int32 ActivePhase = Model.ActivePhase();
    if (ActivePhase != LastActivePhase)
    {
        LastActivePhase = ActivePhase;
        OnSignalPhaseChanged.Broadcast(ActivePhase);
    }
}
