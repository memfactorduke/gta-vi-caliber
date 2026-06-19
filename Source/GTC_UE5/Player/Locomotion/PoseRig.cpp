// Copyright Epic Games, Inc. All Rights Reserved.

#include "PoseRig.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // ln(2): the exact half-life constant shared by both smoothing primitives.
    // Written here rather than at file scope of the header so a shared unity build
    // can never collide on the symbol.
    constexpr double PoseRigLn2 = 0.69314718055994530942;

    /** Wrap an angle into (-PI, PI] for shortest-arc differences (matches the
     *  half-open wrap FAnimRouter uses for facing). */
    double WrapPi(double Value)
    {
        return FMath::Atan2(FMath::Sin(Value), FMath::Cos(Value));
    }

    /** Yaw of a direction in the Godot frame FAnimRouter established: atan2(X, Z),
     *  with Y as the up axis. */
    double DirYaw(const FVector& Dir)
    {
        return FMath::Atan2(Dir.X, Dir.Z);
    }

    /** Pitch of a direction above the X/Z horizontal plane (Y up). */
    double DirPitch(const FVector& Dir)
    {
        const double Planar = FMath::Sqrt(Dir.X * Dir.X + Dir.Z * Dir.Z);
        return FMath::Atan2(Dir.Y, Planar);
    }
}

double FPoseRig::ApproachWeight(double Current, double Target, double Rate, double Delta)
{
    if (Delta <= 0.0 || Rate <= 0.0)
    {
        return Current;
    }
    // Exponential decay of the remaining error: closes (1 - e^-rate*dt) of the gap,
    // so the result is frame-rate independent and never overshoots.
    const double Alpha = 1.0 - FMath::Exp(-Rate * Delta);
    return Current + (Target - Current) * Alpha;
}

double FPoseRig::SpringDamp(double Current, double Target, double& Velocity, double HalfLife, double Delta)
{
    if (Delta <= 0.0)
    {
        return Current;
    }
    // Exact critically-damped spring (the closed-form solution, stable at any dt).
    // y is the damping derived from the half-life; the j0/j1 terms are the analytic
    // position/velocity of a critically damped system integrated over Delta.
    const double Y = PoseRigLn2 / FMath::Max(HalfLife, UE_DOUBLE_SMALL_NUMBER);
    const double J0 = Current - Target;
    const double J1 = Velocity + J0 * Y;
    const double Eydt = FMath::Exp(-Y * Delta);

    Velocity = Eydt * (Velocity - J1 * Y * Delta);
    return Eydt * (J0 + J1 * Delta) + Target;
}

FPoseRig::FStancePose FPoseRig::StancePose(EPosture Posture)
{
    FStancePose Pose;
    switch (Posture)
    {
        case EPosture::Sit:
            // Seated on a chair-height surface: thigh and shin both near a right
            // angle, pelvis dropped to seat height, torso reclined a touch, forearms
            // resting forward on the thighs, gaze level.
            Pose.HipPitch = UE_DOUBLE_HALF_PI;        // ~90 deg thigh flexion
            Pose.KneeFlex = UE_DOUBLE_HALF_PI;        // ~90 deg shin
            Pose.AnklePitch = 0.0;
            Pose.SpinePitch = -0.12;                  // slight recline
            Pose.NeckPitch = 0.05;
            Pose.ShoulderPitch = 0.10;
            Pose.ElbowFlex = 0.45;                    // forearms onto thighs
            Pose.PelvisDrop = -0.45;                  // pelvis lowered to seat height
            Pose.HandToFront = 0.30;
            break;

        case EPosture::Piss:
            // Upright relieving stance: weight settled with soft knees, a small
            // forward pelvis/torso lean, both hands gathered to the body's front
            // centre, head tipped down. Anatomical posing only.
            Pose.HipPitch = 0.05;
            Pose.KneeFlex = 0.15;                     // soft knees
            Pose.AnklePitch = 0.02;
            Pose.SpinePitch = 0.10;                   // slight forward lean
            Pose.NeckPitch = 0.18;                    // looking down
            Pose.ShoulderPitch = 0.15;
            Pose.ElbowFlex = 0.90;                    // forearms forward to centre
            Pose.PelvisDrop = -0.03;
            Pose.HandToFront = 0.85;
            break;

        case EPosture::Locomotion:
        default:
            // Neutral base the gait articulates around. Elbows carry the same resting
            // bend FLocomotion::ElbowFlex uses as its BaseBend so the arms match when
            // the posture blend hands back to the moving gait.
            Pose.ElbowFlex = 0.35;
            break;
    }
    return Pose;
}

FPoseRig::FStancePose FPoseRig::BlendPose(const FStancePose& A, const FStancePose& B, double W)
{
    const double T = FMath::Clamp(W, 0.0, 1.0);
    FStancePose Out;
    Out.HipPitch = FMath::Lerp(A.HipPitch, B.HipPitch, T);
    Out.KneeFlex = FMath::Lerp(A.KneeFlex, B.KneeFlex, T);
    Out.AnklePitch = FMath::Lerp(A.AnklePitch, B.AnklePitch, T);
    Out.SpinePitch = FMath::Lerp(A.SpinePitch, B.SpinePitch, T);
    Out.NeckPitch = FMath::Lerp(A.NeckPitch, B.NeckPitch, T);
    Out.ShoulderPitch = FMath::Lerp(A.ShoulderPitch, B.ShoulderPitch, T);
    Out.ElbowFlex = FMath::Lerp(A.ElbowFlex, B.ElbowFlex, T);
    Out.PelvisDrop = FMath::Lerp(A.PelvisDrop, B.PelvisDrop, T);
    Out.HandToFront = FMath::Lerp(A.HandToFront, B.HandToFront, T);
    return Out;
}

FPoseRig::FTwoBoneSolution FPoseRig::SolveTwoBoneIK(double UpperLen, double LowerLen, double TargetDist)
{
    FTwoBoneSolution Sol;
    if (UpperLen <= 0.0 || LowerLen <= 0.0)
    {
        Sol.JointAngle = UE_DOUBLE_PI; // degenerate bone => leave the limb straight
        Sol.bReachable = false;
        return Sol;
    }

    const double MinReach = FMath::Abs(UpperLen - LowerLen);
    const double MaxReach = UpperLen + LowerLen;
    const double Clamped = FMath::Clamp(TargetDist, MinReach, MaxReach);
    Sol.bReachable = (TargetDist >= MinReach) && (TargetDist <= MaxReach);

    // Law of cosines for the interior angle at the middle joint:
    //   d^2 = u^2 + l^2 - 2*u*l*cos(theta)  =>  theta = acos((u^2 + l^2 - d^2)/(2ul))
    // theta == PI when fully extended (d == u+l), == 0 when folded (d == |u-l|).
    const double CosTheta = (UpperLen * UpperLen + LowerLen * LowerLen - Clamped * Clamped)
                            / (2.0 * UpperLen * LowerLen);
    Sol.JointAngle = FMath::Acos(FMath::Clamp(CosTheta, -1.0, 1.0));
    return Sol;
}

double FPoseRig::FootPlantOffset(double HipHeight, double GroundHeight, double LegLength, double MaxLift)
{
    if (LegLength <= 0.0)
    {
        return 0.0;
    }
    // Where the foot would sit with the leg at its nominal drop, vs. where the
    // ground actually is. Lift toward higher ground, drop into a dip, clamped so a
    // stray trace can't over-extend the leg.
    const double NominalFootHeight = HipHeight - LegLength;
    const double Raw = GroundHeight - NominalFootHeight;
    const double Limit = FMath::Abs(MaxLift);
    return FMath::Clamp(Raw, -Limit, Limit);
}

FPoseRig::FAimAngles FPoseRig::LookAtAngles(
    const FVector& Forward, const FVector& TargetDir, double MaxYaw, double MaxPitch)
{
    FAimAngles Aim;
    if (Forward.IsNearlyZero() || TargetDir.IsNearlyZero())
    {
        return Aim;
    }
    const double DYaw = WrapPi(DirYaw(TargetDir) - DirYaw(Forward));
    const double DPitch = DirPitch(TargetDir) - DirPitch(Forward);
    Aim.Yaw = FMath::Clamp(DYaw, -FMath::Abs(MaxYaw), FMath::Abs(MaxYaw));
    Aim.Pitch = FMath::Clamp(DPitch, -FMath::Abs(MaxPitch), FMath::Abs(MaxPitch));
    return Aim;
}
