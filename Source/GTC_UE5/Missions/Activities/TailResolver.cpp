// Copyright Epic Games, Inc. All Rights Reserved.

#include "TailResolver.h"

#include "Math/UnrealMathUtility.h"

FTailResolver::FOutput FTailResolver::Cook(const FInput& In, const FParams& P)
{
    FOutput Out;

    // Reason on the ground plane (yaw only): a mark on a ramp shouldn't tilt its view cone up or
    // down, and a target on an overpass shouldn't fold its height into the tail distance. UE is
    // Z-up, so the ground plane drops the Z component of both the separation and the facing.
    const FVector ToFollower(In.FollowerPos.X - In.TargetPos.X, In.FollowerPos.Y - In.TargetPos.Y, 0.0);
    Out.Distance = ToFollower.Size();

    // Beyond the mark's seeing range -> out of view whatever the angle.
    if (Out.Distance > FMath::Max(0.0, P.ViewRange))
    {
        Out.bInView = false;
        return Out;
    }

    // Right on top of the mark, or the mark has no facing to speak of -> treat as seen.
    const FVector Fwd = FVector(In.TargetForward.X, In.TargetForward.Y, 0.0).GetSafeNormal();
    if (Out.Distance <= 1.0 || Fwd.SizeSquared() <= 0.0)
    {
        Out.bInView = true;
        return Out;
    }

    // Inside the forward half-angle? The cosine of the angle between the mark's facing and the
    // direction to the follower is high when the follower is in front; past the half-angle the
    // cosine drops below cos(half-angle), i.e. out of the cone.
    const FVector Dir = ToFollower.GetSafeNormal();
    const double CosAngle = FVector::DotProduct(Fwd, Dir);
    const double CosHalf = FMath::Cos(FMath::Max(0.0, P.ViewHalfAngleRad));
    Out.bInView = CosAngle >= CosHalf;
    return Out;
}
