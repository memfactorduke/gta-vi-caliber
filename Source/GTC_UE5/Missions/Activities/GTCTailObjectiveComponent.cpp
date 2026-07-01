// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCTailObjectiveComponent.h"

#include "GameFramework/Actor.h"
#include "Math/UnrealMathUtility.h"

// EGTCTailState mirrors FTailObjective::EState; the tick bridges them with a static_cast, so pin the
// values — reordering either enum breaks the build instead of silently mis-reporting the outcome.
static_assert((uint8)FTailObjective::EState::Tailing == (uint8)EGTCTailState::Tailing, "tail state mirror drift");
static_assert((uint8)FTailObjective::EState::Succeeded == (uint8)EGTCTailState::Succeeded, "tail state mirror drift");
static_assert((uint8)FTailObjective::EState::FailedLost == (uint8)EGTCTailState::FailedLost, "tail state mirror drift");
static_assert((uint8)FTailObjective::EState::FailedSpotted == (uint8)EGTCTailState::FailedSpotted, "tail state mirror drift");

UGTCTailObjectiveComponent::UGTCTailObjectiveComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = false; // inert until StartTail
}

FVector UGTCTailObjectiveComponent::FollowerLocation() const
{
    if (const AActor* F = Follower)
    {
        return F->GetActorLocation();
    }
    if (const AActor* Owner = GetOwner())
    {
        return Owner->GetActorLocation();
    }
    return FVector::ZeroVector;
}

void UGTCTailObjectiveComponent::StartTail()
{
    FTailObjective::FParams P;
    P.MinDistance = MinDistance;
    P.MaxDistance = MaxDistance;
    P.LostGraceSeconds = LostGraceSeconds;
    P.SuspicionRisePerSec = SuspicionRisePerSec;
    P.SuspicionFallPerSec = SuspicionFallPerSec;
    P.RequiredTrackSeconds = RequiredTrackSeconds;
    Tail.Configure(P);

    // Freeze the view-cone tuning for the run too, so all tuning has the same snapshot semantics.
    ViewHalfAngleRadSnapshot = FMath::DegreesToRadians((double)ViewConeHalfAngleDeg);
    ViewRangeSnapshot = ViewRange;

    bActive = true;
    bHadTarget = false;
    Suspicion = 0.0f;
    TrackProgress = 0.0f;
    bInTargetView = false;
    TailState = EGTCTailState::Tailing;
    SetComponentTickEnabled(true);
}

void UGTCTailObjectiveComponent::TickComponent(float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bActive || !Tail.IsTailing())
    {
        return; // no active tail
    }
    if (DeltaTime <= 0.0f)
    {
        return; // a frozen frame passes no time
    }

    const AActor* Mark = Target;
    if (!Mark)
    {
        if (bHadTarget)
        {
            // A mark we were actively tailing has gone (destroyed / GC-nulled) — that's a lost mark,
            // not "no mark assigned yet". Resolve the tail so the mission can't soft-lock.
            bActive = false;
            TailState = EGTCTailState::FailedLost;
            SetComponentTickEnabled(false);
            OnTailFailed.Broadcast(EGTCTailState::FailedLost);
        }
        return; // no mark assigned yet — hold until one is set
    }
    bHadTarget = true;

    FTailResolver::FInput In;
    In.FollowerPos = FollowerLocation();
    In.TargetPos = Mark->GetActorLocation();
    In.TargetForward = Mark->GetActorForwardVector();

    FTailResolver::FParams RP;
    RP.ViewHalfAngleRad = ViewHalfAngleRadSnapshot;
    RP.ViewRange = ViewRangeSnapshot;

    const FTailResolver::FOutput Cooked = FTailResolver::Cook(In, RP);
    Tail.Update(Cooked.Distance, Cooked.bInView, DeltaTime);
    Publish(Cooked.Distance, Cooked.bInView);

    if (!Tail.IsTailing())
    {
        bActive = false;
        SetComponentTickEnabled(false);
        if (Tail.IsSucceeded())
        {
            OnTailSucceeded.Broadcast();
        }
        else
        {
            OnTailFailed.Broadcast(TailState); // FailedLost or FailedSpotted
        }
    }
}

void UGTCTailObjectiveComponent::Publish(double Distance, bool bInView)
{
    Suspicion = (float)Tail.Suspicion();
    TrackProgress = (float)Tail.TrackProgress();
    DistanceToTarget = (float)Distance;
    bInTargetView = bInView;
    TailState = static_cast<EGTCTailState>((uint8)Tail.State());
}
