// Copyright (c) 2026 GTC contributors

#include "StealthDetection.h"

FStealthDetection::FStealthDetection(double InFillRate, double InDecayRate, double InSuspiciousThreshold)
    : FillRate(FMath::Max(InFillRate, 0.0))
    , DecayRate(FMath::Max(InDecayRate, 0.0))
    , SuspiciousThreshold(FMath::Clamp(InSuspiciousThreshold, 0.0001, 0.9999))
{
}

void FStealthDetection::Update(bool bCanSeePlayer, double Visibility, double Delta)
{
    if (Delta <= 0.0)
    {
        return;
    }
    if (bAlerted)
    {
        AwarenessValue = 1.0;
        return;
    }
    if (bCanSeePlayer)
    {
        const double Vis = FMath::Clamp(Visibility, 0.0, 1.0);
        AwarenessValue = FMath::Clamp(AwarenessValue + FillRate * Vis * Delta, 0.0, 1.0);
    }
    else
    {
        AwarenessValue = FMath::Max(AwarenessValue - DecayRate * Delta, 0.0);
    }
    if (AwarenessValue >= 1.0)
    {
        bAlerted = true;
    }
}

double FStealthDetection::Awareness() const
{
    return AwarenessValue;
}

EStealthState FStealthDetection::State() const
{
    if (bAlerted)
    {
        return EStealthState::Alerted;
    }
    if (AwarenessValue >= SuspiciousThreshold)
    {
        return EStealthState::Suspicious;
    }
    return EStealthState::Unaware;
}

bool FStealthDetection::IsAlerted() const
{
    return bAlerted;
}

bool FStealthDetection::IsSuspicious() const
{
    return State() == EStealthState::Suspicious;
}

double FStealthDetection::DetectionSpeed(double Distance, double MaxRange, bool bTargetCrouched, bool bTargetMoving)
{
    if (MaxRange <= 0.0 || Distance >= MaxRange)
    {
        return 0.0;
    }
    double Falloff = 1.0 - FMath::Clamp(Distance, 0.0, MaxRange) / MaxRange;
    if (bTargetCrouched)
    {
        Falloff *= 0.45;
    }
    if (bTargetMoving)
    {
        Falloff *= 1.4;
    }
    return FMath::Clamp(Falloff, 0.0, 1.0);
}

void FStealthDetection::Reset()
{
    AwarenessValue = 0.0;
    bAlerted = false;
}
