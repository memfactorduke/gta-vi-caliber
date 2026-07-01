// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCEscortObjectiveComponent.h"

#include "EscortResolver.h" // FEscortResolver — used only here

#include "GameFramework/Actor.h"

// EGTCEscortState mirrors FEscortObjective::EState; the tick bridges them with a static_cast, so pin
// the values — reordering either enum breaks the build instead of silently mis-reporting the outcome.
static_assert((uint8)FEscortObjective::EState::Escorting == (uint8)EGTCEscortState::Escorting, "escort state mirror drift");
static_assert((uint8)FEscortObjective::EState::Delivered == (uint8)EGTCEscortState::Delivered, "escort state mirror drift");
static_assert((uint8)FEscortObjective::EState::Lost == (uint8)EGTCEscortState::Lost, "escort state mirror drift");

UGTCEscortObjectiveComponent::UGTCEscortObjectiveComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // inert until StartEscort
}

void UGTCEscortObjectiveComponent::StartEscort()
{
    FEscortObjective::FParams P;
    P.DrainPerSec = DrainPerSec;
    P.RegenPerSec = RegenPerSec;
    P.RegenThreatThreshold = RegenThreatThreshold;
    Escort.Configure(P);

    // The route is measured on the first tick with a live escortee (below), NOT here — StartEscort
    // may be called before the convoy actor exists (the tick holds until it's assigned), and a
    // zero-length route would insta-deliver.
    RouteLength = 0.0;
    HighWaterProgress = 0.0;

    // Freeze the resolver geometry for the run (parity with the objective params).
    ArrivalRadiusSnapshot = ArrivalRadius;
    ThreatRadiusSnapshot = ThreatRadius;
    ThreatSaturationSnapshot = ThreatSaturation;

    bActive = true;
    bHadEscortee = false;
    Integrity = 1.0f;
    Progress = 0.0f;
    ThreatLevel = 0.0f;
    EscortState = EGTCEscortState::Escorting;
    SetComponentTickEnabled(true);
}

void UGTCEscortObjectiveComponent::CancelEscort()
{
    bActive = false;
    SetComponentTickEnabled(false);
}

void UGTCEscortObjectiveComponent::ResolveLost()
{
    // The escortee is gone — publish a consistent Lost snapshot (matches the integrity-0 path, which
    // reads Integrity 0), then stop and fire once.
    bActive = false;
    Integrity = 0.0f;
    ThreatLevel = 0.0f;
    EscortState = EGTCEscortState::Lost;
    SetComponentTickEnabled(false);
    OnEscortLost.Broadcast();
}

void UGTCEscortObjectiveComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bActive || !Escort.IsEscorting())
    {
        return; // no active escort
    }
    if (DeltaTime <= 0.0f)
    {
        return; // a frozen frame passes no time
    }

    const AActor* Convoy = Escortee;
    if (!Convoy)
    {
        if (bHadEscortee)
        {
            ResolveLost(); // the convoy we were protecting is gone (destroyed) — escort lost
        }
        return; // else no escortee assigned yet — hold until one is set
    }
    if (!bHadEscortee)
    {
        // First tick with a live escortee — measure the route from where it actually is now.
        RouteLength = FVector::Dist(Convoy->GetActorLocation(), Destination);
        bHadEscortee = true;
    }

    FEscortResolver::FInput In;
    In.EscorteePos = Convoy->GetActorLocation();
    In.Destination = Destination;
    In.RouteLength = RouteLength;
    In.PrevProgress = HighWaterProgress;
    for (const TObjectPtr<AActor>& H : Hostiles)
    {
        if (const AActor* Hostile = H)
        {
            In.HostilePositions.Add(Hostile->GetActorLocation());
        }
    }

    FEscortResolver::FParams RP;
    RP.ArrivalRadius = ArrivalRadiusSnapshot;
    RP.ThreatRadius = ThreatRadiusSnapshot;
    RP.ThreatSaturation = ThreatSaturationSnapshot;

    const FEscortResolver::FOutput Cooked = FEscortResolver::Cook(In, RP);
    HighWaterProgress = Cooked.Progress;

    Escort.Update(Cooked.ProgressDelta, Cooked.ThreatLevel, DeltaTime);
    Publish(Cooked.ThreatLevel);

    if (!Escort.IsEscorting())
    {
        bActive = false;
        SetComponentTickEnabled(false);
        if (Escort.IsDelivered())
        {
            OnEscortDelivered.Broadcast();
        }
        else
        {
            OnEscortLost.Broadcast(); // integrity hit zero
        }
    }
}

void UGTCEscortObjectiveComponent::Publish(double Threat)
{
    Integrity = (float)Escort.Integrity();
    Progress = (float)Escort.Progress();
    ThreatLevel = (float)Threat;
    EscortState = static_cast<EGTCEscortState>((uint8)Escort.State());
}
