// Copyright Epic Games, Inc. All Rights Reserved.

#include "CameraFeel.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    /**
     * Inline port of Godot's `move_toward(a, b, step)`: step `a` toward `b` by at
     * most `abs(step)`, never overshooting. Mirrors Godot semantics exactly.
     */
    float FeelMoveToward(float A, float B, float Step)
    {
        const float Delta = B - A;
        if (FMath::Abs(Delta) <= Step)
        {
            return B;
        }
        return A + FMath::Sign(Delta) * Step;
    }
}

float FCameraFeel::SprintBlend(float Speed, float WalkSpeed, float SprintSpeed)
{
    if (SprintSpeed <= WalkSpeed)
    {
        return 0.0f;
    }
    return FMath::Clamp((Speed - WalkSpeed) / (SprintSpeed - WalkSpeed), 0.0f, 1.0f);
}

float FCameraFeel::FovForBlend(float BaseFov, float Kick, float Blend)
{
    return BaseFov + Kick * Blend;
}

float FCameraFeel::ExpSmoothed(float Current, float Target, float Smoothing, float Delta)
{
    return FMath::Lerp(Current, Target, 1.0f - FMath::Exp(-Smoothing * Delta));
}

float FCameraFeel::RecenterYaw(float VelocityX, float VelocityZ)
{
    if (FMath::IsNearlyZero(VelocityX) && FMath::IsNearlyZero(VelocityZ))
    {
        return 0.0f;
    }
    // Godot atan2(y, x) -> FMath::Atan2(Y, X): recenter_yaw = atan2(-vx, -vz).
    return FMath::Atan2(-VelocityX, -VelocityZ);
}

float FCameraFeel::ApproachAngle(float Current, float Target, float MaxStep)
{
    // wrapf(target - current, -PI, PI) -> shortest signed arc in (-PI, PI].
    // Known non-bit-exact divergence from the Godot oracle at the antipodal
    // (180-off) tie-break: for a diff of exactly +/-PI, FMath::UnwindRadians
    // resolves to +PI (range (-PI, PI]) whereas Godot wrapf(diff, -PI, PI)
    // resolves to -PI (range [-PI, PI)), so the step direction flips. No parity
    // test hits this measure-zero input and the difference is behaviorally
    // negligible, but it is documented here for honesty.
    const float Diff = FMath::UnwindRadians(Target - Current);
    return Current + FMath::Clamp(Diff, -MaxStep, MaxStep);
}

float FCameraFeel::TurnRoll(float YawRate, float Blend, float RollGain, float MaxRoll)
{
    return FMath::Clamp(-YawRate * RollGain, -MaxRoll, MaxRoll) * FMath::Clamp(Blend, 0.0f, 1.0f);
}

FVector2D FCameraFeel::LookOffset(
    const FVector2D& Current,
    const FVector2D& MouseRelative,
    float Sensitivity,
    float YawLimit,
    float PitchMin,
    float PitchMax)
{
    // UNTESTED FOR PARITY — no Godot oracle; verify behaviorally in Wave 3.
    return FVector2D(
        FMath::Clamp(Current.X - MouseRelative.X * Sensitivity, -YawLimit, YawLimit),
        FMath::Clamp(Current.Y - MouseRelative.Y * Sensitivity, PitchMin, PitchMax));
}

FVector2D FCameraFeel::LookReturn(const FVector2D& Current, float Rate, float Delta)
{
    // UNTESTED FOR PARITY — no Godot oracle; verify behaviorally in Wave 3.
    const float Step = Rate * Delta;
    return FVector2D(FeelMoveToward(Current.X, 0.0f, Step), FeelMoveToward(Current.Y, 0.0f, Step));
}
