// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleEntry.h"

int32 FVehicleEntry::NearestAvailableSeat(const TArray<FSeat>& Seats, const FVector& PlayerPos, double Reach)
{
    if (Reach <= 0.0)
    {
        return None;
    }

    int32 Best = None;
    double BestDist = Reach; // inclusive boundary: a seat exactly at Reach still counts
    for (int32 Index = 0; Index < Seats.Num(); ++Index)
    {
        const FSeat& Seat = Seats[Index];
        if (Seat.bOccupied)
        {
            continue;
        }

        const double Dist = FVector::Dist(Seat.Anchor, PlayerPos);
        // Strictly-less keeps ties on the lower index (stable): the driver seat at
        // index 0 wins when the player is equidistant from two front doors.
        if (Dist <= BestDist && (Best == None || Dist < BestDist))
        {
            Best = Index;
            BestDist = Dist;
        }
    }
    return Best;
}

bool FVehicleEntry::BeginEnter(int32 SeatIndex)
{
    if (CurrentState != EState::OnFoot || SeatIndex < 0)
    {
        return false;
    }

    CurrentState = EState::Entering;
    ActiveSeat = SeatIndex;
    Elapsed = 0.0;
    return true;
}

bool FVehicleEntry::BeginExit()
{
    if (CurrentState != EState::Seated)
    {
        return false;
    }

    CurrentState = EState::Exiting;
    Elapsed = 0.0; // keep ActiveSeat so the adapter knows which door to exit
    return true;
}

bool FVehicleEntry::Update(double Dt, const FParams& Params)
{
    if (!InTransition())
    {
        return false;
    }

    Elapsed += FMath::Max(0.0, Dt);

    if (CurrentState == EState::Entering)
    {
        if (Elapsed >= Params.EnterSeconds)
        {
            CurrentState = EState::Seated;
            Elapsed = 0.0;
            return true; // possess the pawn this frame
        }
        return false;
    }

    // Exiting
    if (Elapsed >= Params.ExitSeconds)
    {
        CurrentState = EState::OnFoot;
        ActiveSeat = None;
        Elapsed = 0.0;
        return true; // unpossess the pawn this frame
    }
    return false;
}

double FVehicleEntry::TransitionAlpha(const FParams& Params) const
{
    if (CurrentState == EState::Entering)
    {
        return Params.EnterSeconds > 0.0 ? FMath::Clamp(Elapsed / Params.EnterSeconds, 0.0, 1.0) : 1.0;
    }
    if (CurrentState == EState::Exiting)
    {
        return Params.ExitSeconds > 0.0 ? FMath::Clamp(Elapsed / Params.ExitSeconds, 0.0, 1.0) : 1.0;
    }
    return 0.0;
}
