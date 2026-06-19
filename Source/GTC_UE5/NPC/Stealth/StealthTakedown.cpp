// Copyright Epic Games, Inc. All Rights Reserved.

#include "StealthTakedown.h"

#include "Math/UnrealMathUtility.h"

double FStealthTakedown::RearAngleRad(const FVector& TargetFacing, const FVector& TargetPos, const FVector& AttackerPos)
{
    const FVector Back = (-TargetFacing).GetSafeNormal();
    const FVector ToAttacker = (AttackerPos - TargetPos).GetSafeNormal();
    if (Back.IsNearlyZero() || ToAttacker.IsNearlyZero())
    {
        return PI; // degenerate -> treat as "in front" so silence is never granted on bad input
    }
    const double CosA = FMath::Clamp(FVector::DotProduct(Back, ToAttacker), -1.0, 1.0);
    return FMath::Acos(CosA);
}

bool FStealthTakedown::IsBehind(const FVector& TargetFacing, const FVector& TargetPos, const FVector& AttackerPos,
    double RearArcHalfAngle)
{
    return RearAngleRad(TargetFacing, TargetPos, AttackerPos) <= RearArcHalfAngle;
}

bool FStealthTakedown::InReach(const FVector& AttackerPos, const FVector& TargetPos, double Reach)
{
    if (Reach <= 0.0)
    {
        return false;
    }
    return (AttackerPos - TargetPos).Size() <= Reach;
}

bool FStealthTakedown::CanExecute(const FVector& AttackerPos, const FVector& TargetPos, const FVector& TargetFacing,
    bool bTargetAlerted, double Reach)
{
    // An alerted target is a fight, not a takedown. Otherwise just need to be in reach.
    return !bTargetAlerted && InReach(AttackerPos, TargetPos, Reach);
}

bool FStealthTakedown::IsSilent(const FVector& AttackerPos, const FVector& TargetPos, const FVector& TargetFacing,
    bool bTargetAlerted, double RearArcHalfAngle)
{
    // Quiet only when the victim never saw it coming and it lands from behind.
    return !bTargetAlerted && IsBehind(TargetFacing, TargetPos, AttackerPos, RearArcHalfAngle);
}
