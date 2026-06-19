// Copyright (c) 2026 GTC contributors

#include "AnimRouter.h"

#include "Math/UnrealMathUtility.h"

#include <limits>

const FName FAnimRouter::StateMove(TEXT("Move"));
const FName FAnimRouter::StateJumpStart(TEXT("JumpStart"));
const FName FAnimRouter::StateAir(TEXT("Air"));
const FName FAnimRouter::StateLand(TEXT("Land"));

namespace
{
    /**
     * Inline port of the reference `is_equal_approx(a, b)`: relative tolerance of
     * CMP_EPSILON (1e-5 for 64-bit) scaled by the larger magnitude, with a
     * minimum absolute tolerance so values near zero still compare equal. Used to
     * reproduce the dominant-clip tie-break exactly (float32 blend points make the
     * two competing distances differ only in the last bits).
     */
    bool IsEqualApprox(double A, double B)
    {
        // CMP_EPSILON for the reference 64-bit build.
        constexpr double Epsilon = 0.00001;
        if (A == B)
        {
            return true;
        }
        double Tolerance = Epsilon * FMath::Abs(A);
        if (Tolerance < Epsilon)
        {
            Tolerance = Epsilon;
        }
        return FMath::Abs(A - B) < Tolerance;
    }

    /**
     * Inline port of the reference `wrapf(value, min, max)` for the (-PI, PI] shortest-arc
     * use here: wraps `value` into the half-open range [min, max).
     */
    double Wrapf(double Value, double Min, double Max)
    {
        const double Range = Max - Min;
        if (FMath::IsNearlyZero(Range))
        {
            return Min;
        }
        return Value - (Range * FMath::Floor((Value - Min) / Range));
    }
}

FName FAnimRouter::TravelTarget(
    FLocomotion::EState State, FName Current, double PlanarSpeed, double LandSkipSpeed)
{
    switch (State)
    {
        case FLocomotion::EState::Jump:
            if (Current == StateAir || Current == StateJumpStart)
            {
                return Current;
            }
            return StateJumpStart;

        case FLocomotion::EState::Fall:
            return Current == StateJumpStart ? StateJumpStart : StateAir;

        case FLocomotion::EState::Climb:
            return StateMove;

        default:
            if (Current == StateAir || Current == StateJumpStart)
            {
                return PlanarSpeed >= LandSkipSpeed ? StateMove : StateLand;
            }
            if (Current == StateLand)
            {
                return StateLand;
            }
            return StateMove;
    }
}

double FAnimRouter::MoveBlendValue(
    double PlanarSpeed, double TotalSpeed, bool bIsClimbing, double WalkSpeed, double RunSpeed)
{
    const double Speed = bIsClimbing ? TotalSpeed : PlanarSpeed;
    return FLocomotion::MoveBlend(Speed, WalkSpeed, RunSpeed);
}

double FAnimRouter::DominantBlendPoint(double Blend, const TArray<float>& Points)
{
    double Best = std::numeric_limits<double>::quiet_NaN();
    double BestDistance = std::numeric_limits<double>::infinity();
    for (const float Point : Points)
    {
        // Mirror Godot: the points come from a PackedFloat32Array, so the subtraction
        // promotes the float32 point to double exactly as in the oracle.
        const double Distance = FMath::Abs(Blend - static_cast<double>(Point));
        if (Distance < BestDistance || IsEqualApprox(Distance, BestDistance))
        {
            Best = static_cast<double>(Point);
            BestDistance = Distance;
        }
    }
    return Best;
}

double FAnimRouter::RotateTowardAngle(double Current, double Target, double MaxStep)
{
    const double Diff = Wrapf(Target - Current, -UE_DOUBLE_PI, UE_DOUBLE_PI);
    return Current + FMath::Clamp(Diff, -MaxStep, MaxStep);
}

double FAnimRouter::FacingTarget(const FVector& PlanarVelocity, double AimYaw)
{
    if (!FMath::IsNaN(AimYaw))
    {
        return AimYaw;
    }
    const double PlanarSpeed = PlanarVelocity.Size();
    if (PlanarSpeed > FLocomotion::IdleSpeedEpsilon)
    {
        return FMath::Atan2(PlanarVelocity.X, PlanarVelocity.Z);
    }
    return std::numeric_limits<double>::quiet_NaN();
}
