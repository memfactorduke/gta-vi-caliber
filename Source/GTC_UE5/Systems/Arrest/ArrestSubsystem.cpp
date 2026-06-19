// Copyright (c) 2026 GTC contributors

#include "ArrestSubsystem.h"

void UArrestSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    _TimeHeld = 0.0;
}

void UArrestSubsystem::Deinitialize()
{
    Super::Deinitialize();
}

bool UArrestSubsystem::Tick(int32 Stars, double NearestCopDistance, double DeltaSeconds)
{
    // Mirrors ArrestController._physics_process: cornered -> tick grapple -> maybe bust.
    const bool bCornered = FArrestModel::Cornered(Stars, NearestCopDistance, CatchDistance);
    _TimeHeld = FArrestModel::TickGrapple(_TimeHeld, bCornered, DeltaSeconds);
    if (FArrestModel::IsBusted(_TimeHeld, GrappleTime))
    {
        ApplyBust();
        return true;
    }
    return false;
}

double UArrestSubsystem::GrappleProgress() const
{
    // Godot: clampf(_time_held / grapple_time, 0.0, 1.0) if grapple_time > 0.0 else 0.0.
    return GrappleTime > 0.0 ? FMath::Clamp(_TimeHeld / GrappleTime, 0.0, 1.0) : 0.0;
}

void UArrestSubsystem::ApplyBust()
{
    // Mirrors ArrestController._apply_bust, but writes to OWNED state (deferred coupling):
    // reset the grapple, take the fee from the owned wallet snapshot, request a heat clear
    // for the Wave 3 Wanted adapter to drain, and fire the busted delegate.
    _TimeHeld = 0.0;
    _LastFee = FArrestModel::BustFee(_Wallet, CashPenalty);
    _Wallet = FArrestModel::CashAfterBust(_Wallet, CashPenalty);
    bClearHeatRequested = true;
    OnBusted.Broadcast(_LastFee);
}
