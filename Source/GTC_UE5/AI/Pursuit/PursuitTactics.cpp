// Copyright Epic Games, Inc. All Rights Reserved.

#include "PursuitTactics.h"

#include "Math/UnrealMathUtility.h"

// PI * 30 / 180 — the Godot RAM_ARC_HALF constant. UE_DOUBLE_PI used directly per
// unity-build hygiene (no file-scope common-named const aliasing).
const double FPursuitTactics::RamArcHalf = UE_DOUBLE_PI * 30.0 / 180.0;

FVector FPursuitTactics::Ground(const FVector& V)
{
    return FVector(V.X, 0.0, V.Z);
}

FVector FPursuitTactics::PlanarDir(const FVector& A, const FVector& B)
{
    const FVector D = Ground(B - A);
    return D.Size() > Eps ? D.GetSafeNormal() : FVector::ZeroVector;
}

FVector FPursuitTactics::InterceptPoint(
    const FVector& TargetPos, const FVector& TargetVel, const FVector& PursuerPos,
    double PursuerSpeed)
{
    const FVector Tv = Ground(TargetVel);
    const FVector Tp = Ground(TargetPos);
    const FVector Pp = Ground(PursuerPos);
    // Nothing to lead if the target is still or we can't chase.
    if (Tv.Size() < Eps || PursuerSpeed <= Eps)
    {
        return Tp;
    }
    const FVector ToTarget = Tp - Pp;
    // Solve |ToTarget + Tv * t| = PursuerSpeed * t for the smallest t > 0.
    const double A = Tv.Dot(Tv) - PursuerSpeed * PursuerSpeed;
    const double B = 2.0 * ToTarget.Dot(Tv);
    const double C = ToTarget.Dot(ToTarget);
    double T = -1.0;
    if (FMath::Abs(A) < Eps)
    {
        // Linear case (target speed ≈ pursuer speed).
        if (FMath::Abs(B) > Eps)
        {
            T = -C / B;
        }
    }
    else
    {
        const double Disc = B * B - 4.0 * A * C;
        if (Disc >= 0.0)
        {
            const double Root = FMath::Sqrt(Disc);
            const double T1 = (-B - Root) / (2.0 * A);
            const double T2 = (-B + Root) / (2.0 * A);
            // Smallest strictly-positive root.
            T = SmallestPositive(T1, T2);
        }
    }
    if (T <= 0.0)
    {
        return Tp;
    }
    return Tp + Tv * T;
}

double FPursuitTactics::SmallestPositive(double T1, double T2)
{
    const double Lo = FMath::Min(T1, T2);
    const double Hi = FMath::Max(T1, T2);
    if (Lo > Eps)
    {
        return Lo;
    }
    if (Hi > Eps)
    {
        return Hi;
    }
    return -1.0;
}

bool FPursuitTactics::ShouldRam(
    const FVector& PursuerPos, const FVector& PursuerHeading, const FVector& TargetPos,
    double RamRange, int32 Stars)
{
    if (Stars < AggressionStars)
    {
        return false;
    }
    const FVector ToTarget = Ground(TargetPos - PursuerPos);
    const double Dist = ToTarget.Size();
    if (Dist < Eps || Dist > RamRange)
    {
        return false;
    }
    const FVector Heading = Ground(PursuerHeading);
    if (Heading.Size() < Eps)
    {
        return false;
    }
    return Heading.GetSafeNormal().Dot(ToTarget.GetSafeNormal()) >= FMath::Cos(RamArcHalf);
}

FVector FPursuitTactics::BlockOffset(
    const FVector& TargetPos, const FVector& TargetVel, double Side, double Distance)
{
    const FVector Tp = Ground(TargetPos);
    const FVector Fwd = Ground(TargetVel);
    const double S = FMath::Abs(Side) > Eps ? FMath::Sign(Side) : 1.0;
    if (Fwd.Size() < Eps)
    {
        // No travel direction: just step out to the requested side along +Z.
        return Tp + FVector(0.0, 0.0, S) * Distance;
    }
    const FVector Dir = Fwd.GetSafeNormal();
    // Right-hand perpendicular on the XZ plane.
    const FVector Right = FVector(Dir.Z, 0.0, -Dir.X);
    // Mostly ahead, partly to the side, so the block sits in the target's path.
    const FVector Ahead = Dir * Distance;
    const FVector Lateral = Right * S * (Distance * 0.5);
    return Tp + Ahead + Lateral;
}

double FPursuitTactics::PitSide(
    const FVector& PursuerPos, const FVector& TargetPos, const FVector& TargetVel)
{
    const FVector Fwd = Ground(TargetVel);
    if (Fwd.Size() < Eps)
    {
        return 1.0;
    }
    const FVector Dir = Fwd.GetSafeNormal();
    const FVector ToPursuer = Ground(PursuerPos - TargetPos);
    if (ToPursuer.Size() < Eps)
    {
        return 1.0;
    }
    // Right-hand perpendicular; positive projection → pursuer is on the right.
    const FVector Right = FVector(Dir.Z, 0.0, -Dir.X);
    const double Lateral = ToPursuer.Dot(Right);
    if (FMath::Abs(Lateral) < Eps)
    {
        return 1.0;
    }
    return FMath::Sign(Lateral);
}

double FPursuitTactics::DesiredSpeed(double DistanceToTarget, double BaseSpeed, double MaxSpeed)
{
    const double D = FMath::Max(DistanceToTarget, 0.0);
    // Right on the bumper: ease off to avoid overshooting past the target.
    const double TuckIn = 6.0;
    if (D < TuckIn)
    {
        return FMath::Lerp(BaseSpeed * 0.5, BaseSpeed, D / TuckIn);
    }
    // Beyond the tuck-in zone, climb toward max as the gap widens.
    const double FullChase = 40.0;
    const double T = FMath::Clamp((D - TuckIn) / (FullChase - TuckIn), 0.0, 1.0);
    return FMath::Lerp(BaseSpeed, MaxSpeed, T);
}

bool FPursuitTactics::ShouldBackOff(int32 Stars, double Distance)
{
    return Stars <= 0 || Distance > GiveUpRange;
}

EPursuitTactic FPursuitTactics::ChooseTactic(
    const FVector& PursuerPos, const FVector& PursuerHeading, const FVector& TargetPos,
    const FVector& TargetVel, int32 Stars, double RamRange)
{
    const double Dist = Ground(TargetPos - PursuerPos).Size();
    if (ShouldBackOff(Stars, Dist))
    {
        return EPursuitTactic::BackOff;
    }
    if (ShouldRam(PursuerPos, PursuerHeading, TargetPos, RamRange, Stars))
    {
        return EPursuitTactic::Ram;
    }
    // Authorised contact but not lined up head-on: if alongside the target swing
    // for a PIT; otherwise wall it off with a block.
    if (Stars >= AggressionStars && Dist <= RamRange * 2.0)
    {
        if (IsAlongside(PursuerPos, TargetPos, TargetVel))
        {
            return EPursuitTactic::Pit;
        }
        return EPursuitTactic::Block;
    }
    return EPursuitTactic::Chase;
}

bool FPursuitTactics::IsAlongside(
    const FVector& PursuerPos, const FVector& TargetPos, const FVector& TargetVel)
{
    const FVector Fwd = Ground(TargetVel);
    if (Fwd.Size() < Eps)
    {
        return false;
    }
    const FVector Dir = Fwd.GetSafeNormal();
    const FVector ToPursuer = Ground(PursuerPos - TargetPos);
    if (ToPursuer.Size() < Eps)
    {
        return false;
    }
    const FVector Right = FVector(Dir.Z, 0.0, -Dir.X);
    const double Along = ToPursuer.Dot(Dir);
    const double Lateral = ToPursuer.Dot(Right);
    return FMath::Abs(Lateral) >= FMath::Abs(Along);
}
