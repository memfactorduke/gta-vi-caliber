// Copyright Epic Games, Inc. All Rights Reserved.

#include "ArrestModel.h"

bool FArrestModel::Cornered(int32 Stars, double Distance, double CatchDistance)
{
    // Godot: stars > 0 and distance <= catch_distance.
    return Stars > 0 && Distance <= CatchDistance;
}

double FArrestModel::TickGrapple(double TimeHeld, bool bIsCornered, double Dt)
{
    // Godot: maxf(time_held + dt, 0.0) if is_cornered else maxf(time_held - dt, 0.0).
    return bIsCornered ? FMath::Max(TimeHeld + Dt, 0.0)
                       : FMath::Max(TimeHeld - Dt, 0.0);
}

bool FArrestModel::IsBusted(double TimeHeld, double GrappleTime)
{
    // Godot: grapple_time > 0.0 and time_held >= grapple_time.
    return GrappleTime > 0.0 && TimeHeld >= GrappleTime;
}

int32 FArrestModel::CashAfterBust(int32 Wallet, double PenaltyFraction)
{
    // Godot: kept = float(wallet) * (1.0 - clampf(penalty_fraction, 0.0, 1.0));
    //        return maxi(floori(kept), 0).
    // floori = floor toward -inf; FMath::FloorToInt32 matches for these non-negative kepts,
    // and we floor the result at 0 to match maxi(..., 0).
    const double Kept = static_cast<double>(Wallet) * (1.0 - FMath::Clamp(PenaltyFraction, 0.0, 1.0));
    return FMath::Max(FMath::FloorToInt32(Kept), 0);
}

int32 FArrestModel::BustFee(int32 Wallet, double PenaltyFraction)
{
    // Godot: maxi(wallet - cash_after_bust(wallet, penalty_fraction), 0).
    return FMath::Max(Wallet - CashAfterBust(Wallet, PenaltyFraction), 0);
}
