// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCDeliveryRunComponent.h"

#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

// The BP-facing EGTCDeliveryState mirrors the pure core's FDeliveryRun::EState; the tick bridges
// them with a static_cast, so pin the values here — reordering either enum breaks the build
// instead of silently mis-reporting the delivery outcome.
static_assert((uint8)FDeliveryRun::EState::InProgress == (uint8)EGTCDeliveryState::InProgress, "delivery state mirror drift");
static_assert((uint8)FDeliveryRun::EState::Delivered == (uint8)EGTCDeliveryState::Delivered, "delivery state mirror drift");
static_assert((uint8)FDeliveryRun::EState::TooSlow == (uint8)EGTCDeliveryState::TooSlow, "delivery state mirror drift");
static_assert((uint8)FDeliveryRun::EState::Wrecked == (uint8)EGTCDeliveryState::Wrecked, "delivery state mirror drift");

UGTCDeliveryRunComponent::UGTCDeliveryRunComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
}

FVector UGTCDeliveryRunComponent::CourierLocation() const
{
    if (const AActor* Tracked = Courier)
    {
        return Tracked->GetActorLocation();
    }
    if (const AActor* Owner = GetOwner())
    {
        return Owner->GetActorLocation();
    }
    return FVector::ZeroVector;
}

void UGTCDeliveryRunComponent::StartRunTo(const FVector& Dropoff, float TimeLimit)
{
    DropoffLocation = Dropoff;
    TimeLimitSeconds = FMath::Max(1.0f, TimeLimit);
    RouteLength = FVector::Dist(CourierLocation(), DropoffLocation);
    HighWaterProgress = 0.0;
    PendingImpulse = 0.0f;

    FDeliveryRun::FParams P;
    P.TimeLimitSeconds = TimeLimitSeconds;
    Run.Configure(P);

    bActive = true;
    Publish(RouteLength); // at the start the distance to the drop is the whole route
}

void UGTCDeliveryRunComponent::StartRun()
{
    StartRunTo(DropoffLocation, TimeLimitSeconds);
}

void UGTCDeliveryRunComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bActive || !Run.IsInProgress())
    {
        return; // no active run — a single check, no per-frame work
    }
    if (DeltaTime <= 0.0f)
    {
        return; // a frozen / zero-delta frame passes no time; don't bank progress or eat impacts
    }

    // Cook world state into the delivery core's inputs (monotonic route progress + cargo damage).
    const double DistToDrop = FVector::Dist(CourierLocation(), DropoffLocation);
    FDeliveryRunResolver::FInput In;
    In.DistanceToDropoff = DistToDrop;
    In.RouteLength = RouteLength;
    In.PrevProgress = HighWaterProgress;
    In.ImpactImpulse = PendingImpulse;

    FDeliveryRunResolver::FParams RP;
    RP.DamagePerImpulse = DamagePerImpulse;
    RP.ArrivalRadius = ArrivalRadius;

    const FDeliveryRunResolver::FOutput Cooked = FDeliveryRunResolver::Cook(In, RP);
    HighWaterProgress = Cooked.Progress;
    PendingImpulse = 0.0f; // consumed this tick

    Run.Update(Cooked.ProgressDelta, Cooked.Damage, DeltaTime);
    Publish(DistToDrop);

    // On the frame the run resolves, fire the matching event exactly once and stop.
    if (!Run.IsInProgress())
    {
        bActive = false;
        if (Run.IsDelivered())
        {
            OnDeliveryCompleted.Broadcast((float)Run.PayoutFactor());
        }
        else
        {
            OnDeliveryFailed.Broadcast(DeliveryState); // TooSlow or Wrecked
        }
    }
}

void UGTCDeliveryRunComponent::Publish(double DistToDrop)
{
    Progress = (float)Run.Progress();
    TimeLeftSeconds = (float)Run.TimeLeft();
    CargoCondition = (float)Run.CargoCondition();
    PayoutFactor = (float)Run.PayoutFactor();
    DistanceToDropoff = (float)DistToDrop;
    DeliveryState = static_cast<EGTCDeliveryState>((uint8)Run.State());
}
