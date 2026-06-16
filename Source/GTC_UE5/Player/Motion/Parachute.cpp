// Copyright Epic Games, Inc. All Rights Reserved.

#include "Parachute.h"

#include "Math/UnrealMathUtility.h"

#include <limits>

FParachute::FParachute(double TerminalVelocity, double CanopyDescentRate)
{
    TerminalVelocityValue = FMath::Max(TerminalVelocity, 0.0);
    CanopyDescentRateValue = FMath::Max(CanopyDescentRate, 0.0);
}

bool FParachute::Deploy()
{
    if (StateValue != EState::Freefall)
    {
        return false;
    }
    StateValue = EState::Deployed;
    return true;
}

bool FParachute::IsDeployed() const
{
    return StateValue == EState::Deployed;
}

FParachute::EState FParachute::State() const
{
    return StateValue;
}

double FParachute::UpdateFallSpeed(double CurrentSpeed, double Delta) const
{
    double Speed = FMath::Max(CurrentSpeed, 0.0);
    const double Dt = FMath::Max(Delta, 0.0);
    if (StateValue == EState::Deployed)
    {
        if (Speed > CanopyDescentRateValue)
        {
            Speed = FMath::Max(Speed - CanopyDrag * Dt, CanopyDescentRateValue);
        }
        else
        {
            Speed = FMath::Min(Speed + CanopyDrag * Dt, CanopyDescentRateValue);
        }
        return FMath::Clamp(Speed, 0.0, TerminalVelocityValue);
    }
    // FREEFALL (and LANDED, harmlessly): accelerate toward terminal.
    Speed = FMath::Min(Speed + Gravity * Dt, TerminalVelocityValue);
    return FMath::Clamp(Speed, 0.0, TerminalVelocityValue);
}

FVector FParachute::HorizontalDrift(const FVector& SteerInput, bool bDeployed, double GlideSpeed) const
{
    const FVector Flat(SteerInput.X, 0.0, SteerInput.Z);
    if (Flat.Size() < 0.0001)
    {
        return FVector::ZeroVector;
    }
    const double Glide = FMath::Max(GlideSpeed, 0.0);
    // Freefall body-flying gives only a sliver of the canopy's authority.
    const double Authority = bDeployed ? Glide : Glide * 0.12;
    FVector Drift = Flat.GetSafeNormal() * Authority;
    if (Drift.Size() > Glide)
    {
        Drift = Drift.GetSafeNormal() * Glide;
    }
    return Drift;
}

double FParachute::LandingImpact(double VerticalSpeed, double SafeSpeed) const
{
    const double V = FMath::Max(VerticalSpeed, 0.0);
    const double Safe = FMath::Max(SafeSpeed, 0.0);
    if (V <= Safe)
    {
        return 0.0;
    }
    if (Safe < 0.0001)
    {
        return 1.0;
    }
    // Excess speed beyond safe, normalised over another `safe` band -> full damage.
    return FMath::Clamp((V - Safe) / Safe, 0.0, 1.0);
}

bool FParachute::IsSafeLanding(double VerticalSpeed, double SafeSpeed) const
{
    return FMath::Max(VerticalSpeed, 0.0) <= FMath::Max(SafeSpeed, 0.0);
}

double FParachute::TimeToGround(double Altitude, double DescentRate) const
{
    const double Alt = FMath::Max(Altitude, 0.0);
    if (Alt <= 0.0)
    {
        return 0.0;
    }
    if (DescentRate <= 0.0001)
    {
        return std::numeric_limits<double>::infinity();
    }
    return Alt / DescentRate;
}

void FParachute::Land()
{
    StateValue = EState::Landed;
}

void FParachute::Reset()
{
    StateValue = EState::Freefall;
}
