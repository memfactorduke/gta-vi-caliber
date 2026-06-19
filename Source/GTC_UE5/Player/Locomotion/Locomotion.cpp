// Copyright (c) 2026 GTC contributors

#include "Locomotion.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // the reference TAU/PI as doubles, matching the reference implementation's 64-bit float constants. These
    // are written inline as (2.0 * UE_DOUBLE_PI) and UE_DOUBLE_PI at each call
    // site: a file-scope constant is NOT unity-isolated and could redefine
    // another TU's same-named symbol in a shared unity build.

    /**
     * Inline port of the reference `fposmod(x, y)`: a modulo whose result carries the
     * sign of the divisor y, so the wrapped phase always lands in [0, y) for the
     * positive y used here. Mirrors the reference semantics exactly.
     */
    double Fposmod(double X, double Y)
    {
        double R = FMath::Fmod(X, Y);
        if (R != 0.0 && ((Y < 0.0) != (R < 0.0)))
        {
            R += Y;
        }
        return R;
    }
}

FLocomotion::EState FLocomotion::StateFor(
    double PlanarSpeed,
    bool bOnFloor,
    double VerticalVelocity,
    bool bIsClimbing,
    double WalkSpeed,
    double RunSpeed)
{
    if (bIsClimbing)
    {
        return EState::Climb;
    }
    if (!bOnFloor)
    {
        return VerticalVelocity > 0.0 ? EState::Jump : EState::Fall;
    }
    if (PlanarSpeed < IdleSpeedEpsilon)
    {
        return EState::Idle;
    }
    // lerpf(walk_speed, run_speed, 0.5)
    const double RunGate = WalkSpeed + (RunSpeed - WalkSpeed) * 0.5;
    return PlanarSpeed > RunGate ? EState::Run : EState::Walk;
}

double FLocomotion::MoveBlend(double PlanarSpeed, double WalkSpeed, double RunSpeed)
{
    if (PlanarSpeed <= 0.0)
    {
        return 0.0;
    }
    if (PlanarSpeed <= WalkSpeed)
    {
        return 0.5 * (PlanarSpeed / WalkSpeed);
    }
    if (PlanarSpeed >= RunSpeed)
    {
        return 1.0;
    }
    const double T = (PlanarSpeed - WalkSpeed) / (RunSpeed - WalkSpeed);
    return 0.5 + 0.5 * T;
}

double FLocomotion::AdvancePhase(double Phase, double PlanarSpeed, double Delta)
{
    const double Advance = PlanarSpeed * Delta * StrideCyclesPerMetre * (2.0 * UE_DOUBLE_PI);
    return Fposmod(Phase + Advance, (2.0 * UE_DOUBLE_PI));
}

double FLocomotion::LimbSwing(double Phase, double Amplitude)
{
    return FMath::Sin(Phase) * Amplitude;
}

double FLocomotion::VerticalBob(double Phase, double Amplitude)
{
    return -FMath::Abs(FMath::Sin(Phase)) * Amplitude;
}

double FLocomotion::LateralSway(double Phase, double Amplitude)
{
    return FMath::Sin(Phase) * Amplitude;
}

double FLocomotion::PelvisRoll(double Phase, double Amplitude)
{
    return -FMath::Sin(Phase) * Amplitude;
}

double FLocomotion::ShoulderCounterRoll(double Phase, double Amplitude)
{
    return FMath::Sin(Phase) * Amplitude;
}

double FLocomotion::TorsoTwist(double Phase, double Amplitude)
{
    return -FMath::Sin(Phase) * Amplitude;
}

double FLocomotion::HeadStepPitch(double Phase, double Amplitude)
{
    return -FMath::Abs(FMath::Sin(Phase)) * Amplitude;
}

double FLocomotion::HeadCounterRoll(double Phase, double Amplitude)
{
    return -FMath::Sin(Phase) * Amplitude;
}

double FLocomotion::IdleBreath(double IdleTime, double Amplitude)
{
    return FMath::Sin(IdleTime * (2.0 * UE_DOUBLE_PI) * 0.33) * Amplitude;
}

double FLocomotion::IdleWeightShift(double IdleTime, double Amplitude)
{
    return FMath::Sin(IdleTime * (2.0 * UE_DOUBLE_PI) * 0.18) * Amplitude;
}

double FLocomotion::IdleHeadPitch(double IdleTime, double Amplitude)
{
    return -FMath::Sin(IdleTime * (2.0 * UE_DOUBLE_PI) * 0.33 + UE_DOUBLE_PI * 0.18) * Amplitude;
}

double FLocomotion::FootToeOut(double Side, double SwingNorm, double Base, double Swing)
{
    return Side * (Base + FMath::Max(0.0, -SwingNorm) * Swing);
}

double FLocomotion::FootBank(double Side, double SwingNorm, double Amplitude)
{
    return -Side * SwingNorm * Amplitude;
}

double FLocomotion::TurnLean(double AngularVelocity, double Reference, double MaxLean)
{
    if (Reference <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(AngularVelocity / Reference, -1.0, 1.0) * MaxLean;
}

double FLocomotion::LandingCompression(double PreviousVerticalVelocity, double Reference, double MaxCompression)
{
    if (Reference <= 0.0 || PreviousVerticalVelocity >= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(FMath::Abs(PreviousVerticalVelocity) / Reference, 0.0, 1.0) * MaxCompression;
}

double FLocomotion::LeanAngle(double Accel, double AccelReference, double MaxLean)
{
    if (AccelReference <= 0.0)
    {
        return 0.0;
    }
    return FMath::Clamp(Accel / AccelReference, -1.0, 1.0) * MaxLean;
}

// --- Articulated gait -------------------------------------------------------

double FLocomotion::KneeFlex(double Phase, double Amplitude, double StanceFlex)
{
    const double Swing = FMath::Max(0.0, -FMath::Sin(Phase)); // positive only through swing
    return StanceFlex + Amplitude * FMath::Pow(Swing, 1.3);
}

double FLocomotion::AnklePitch(double Phase, double Amplitude)
{
    return FMath::Sin(Phase + UE_DOUBLE_PI * 0.5) * Amplitude;
}

double FLocomotion::ElbowFlex(double ArmPhase, double Amplitude, double BaseBend)
{
    return BaseBend + Amplitude * FMath::Max(0.0, FMath::Sin(ArmPhase));
}

double FLocomotion::KneeFlexFromSwing(double SwingNorm, double Amplitude, double StanceFlex)
{
    return StanceFlex + Amplitude * FMath::Pow(FMath::Max(0.0, -SwingNorm), 1.3);
}

double FLocomotion::ElbowFlexFromSwing(double SwingNorm, double Amplitude, double BaseBend)
{
    return BaseBend + Amplitude * FMath::Max(0.0, SwingNorm);
}
