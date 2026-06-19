// Copyright (c) 2026 GTC contributors

#include "PlayerMotion.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    /**
     * Move From toward To by at most Delta, never overshooting (clamps to To
     * once within Delta). Delta is expected non-negative; callers pass
     * Accel * DeltaTime. Equivalent to FMath::FInterpConstantTo with the step
     * precomputed.
     */
    double MoveToward(double From, double To, double Delta)
    {
        const double Diff = To - From;
        if (FMath::Abs(Diff) <= Delta)
        {
            return To;
        }
        return From + FMath::Sign(Diff) * Delta;
    }
}

FVector FPlayerMotion::DirectionFromInput(const FVector2D& InputDir, double CameraYaw)
{
    // Treat a near-zero input vector as no input.
    if (InputDir.IsNearlyZero())
    {
        return FVector::ZeroVector;
    }
    // Local: (x, 0, y). Rotate about +Y (UP) by CameraYaw (right-handed):
    //   x' =  x*cos + z*sin
    //   z' = -x*sin + z*cos
    const double LocalX = InputDir.X;
    const double LocalZ = InputDir.Y;
    const double C = FMath::Cos(CameraYaw);
    const double S = FMath::Sin(CameraYaw);
    const FVector Rotated(LocalX * C + LocalZ * S, 0.0, -LocalX * S + LocalZ * C);
    return Rotated.GetSafeNormal();
}

FVector FPlayerMotion::HorizontalVelocity(const FVector& Direction, double Speed)
{
    return FVector(Direction.X * Speed, 0.0, Direction.Z * Speed);
}

FVector FPlayerMotion::Accelerated(const FVector& Current, const FVector& Target, double Acceleration, double Delta)
{
    const double Step = Acceleration * Delta;
    return FVector(
        MoveToward(Current.X, Target.X, Step),
        Current.Y,
        MoveToward(Current.Z, Target.Z, Step));
}

double FPlayerMotion::AccelerationRate(bool bHasInput, bool bOnFloor, double Accel, double Decel, double AirControl)
{
    double Rate = bHasInput ? Accel : Decel;
    if (!bOnFloor)
    {
        Rate *= AirControl;
    }
    return Rate;
}

bool FPlayerMotion::ShouldJump(
    double TimeSinceGrounded,
    double CoyoteTime,
    double TimeSinceJumpPressed,
    double BufferTime,
    bool bJumpSpent)
{
    if (bJumpSpent)
    {
        return false;
    }
    return TimeSinceGrounded <= CoyoteTime && TimeSinceJumpPressed <= BufferTime;
}

double FPlayerMotion::FallDamage(double ImpactSpeed, double SafeSpeed, double LethalSpeed, double MaxDamage)
{
    if (ImpactSpeed <= SafeSpeed || LethalSpeed <= SafeSpeed)
    {
        return 0.0;
    }
    return FMath::Clamp((ImpactSpeed - SafeSpeed) / (LethalSpeed - SafeSpeed), 0.0, 1.0) * MaxDamage;
}

FVector FPlayerMotion::SlopeSlide(const FVector& FloorNormal, double MaxWalkNormalY, double SlideAccel)
{
    if (FloorNormal.Y >= MaxWalkNormalY || FloorNormal.Y >= 1.0)
    {
        return FVector::ZeroVector;
    }
    const FVector Downhill(FloorNormal.X, 0.0, FloorNormal.Z);
    if (Downhill.Size() < 0.0001)
    {
        return FVector::ZeroVector;
    }
    const double Steepness = FMath::Clamp(
        (MaxWalkNormalY - FloorNormal.Y) / FMath::Max(MaxWalkNormalY, 0.0001), 0.0, 1.0);
    return Downhill.GetSafeNormal() * SlideAccel * Steepness;
}

FVector FPlayerMotion::ClimbVelocity(const FVector2D& InputDir, const FVector& Direction, double ClimbSpeed)
{
    return FVector(
        Direction.X * ClimbSpeed * 0.5,
        -InputDir.Y * ClimbSpeed,
        Direction.Z * ClimbSpeed * 0.5);
}
