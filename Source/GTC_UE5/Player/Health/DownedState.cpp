// Copyright Epic Games, Inc. All Rights Reserved.

#include "DownedState.h"

void FDownedState::Configure(const FParams& InParams)
{
    Params = InParams;
    CurrentState = EState::Up;
    BleedTimer = 0.0;
    Downs = 0;
}

void FDownedState::GoDown()
{
    if (CurrentState != EState::Up)
    {
        return; // already down or dead
    }

    const int32 Budget = FMath::Max(0, Params.MaxDowns);
    if (Downs >= Budget)
    {
        // Out of second chances — this drop is fatal.
        CurrentState = EState::Dead;
        return;
    }

    CurrentState = EState::Downed;
    BleedTimer = 0.0;
    ++Downs;
}

void FDownedState::Revive()
{
    if (CurrentState == EState::Downed)
    {
        CurrentState = EState::Up;
        BleedTimer = 0.0;
    }
}

void FDownedState::FinishOff()
{
    if (CurrentState == EState::Downed)
    {
        CurrentState = EState::Dead;
    }
}

void FDownedState::Tick(double Dt)
{
    if (CurrentState != EState::Downed)
    {
        return;
    }

    BleedTimer += FMath::Max(0.0, Dt);
    if (BleedTimer >= FMath::Max(0.0, Params.BleedOutSeconds))
    {
        CurrentState = EState::Dead;
    }
}

void FDownedState::Reset()
{
    CurrentState = EState::Up;
    BleedTimer = 0.0;
    Downs = 0;
}

double FDownedState::BleedProgress() const
{
    if (CurrentState != EState::Downed)
    {
        return 0.0;
    }
    const double Window = FMath::Max(0.0, Params.BleedOutSeconds);
    return Window > 0.0 ? FMath::Clamp(BleedTimer / Window, 0.0, 1.0) : 1.0;
}

double FDownedState::TimeLeft() const
{
    if (CurrentState != EState::Downed)
    {
        return 0.0;
    }
    return FMath::Max(0.0, FMath::Max(0.0, Params.BleedOutSeconds) - BleedTimer);
}
