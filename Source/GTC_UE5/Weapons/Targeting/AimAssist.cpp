// Copyright Epic Games, Inc. All Rights Reserved.

#include "AimAssist.h"

#include "Math/UnrealMathUtility.h"

double FAimAssist::AngleToTargetRad(const FVector& AimOrigin, const FVector& AimDir, const FVector& TargetPos)
{
    const FVector To = (TargetPos - AimOrigin).GetSafeNormal();
    const FVector Aim = AimDir.GetSafeNormal();
    if (To.IsNearlyZero() || Aim.IsNearlyZero())
    {
        return 0.0; // degenerate — nothing to deflect toward
    }
    const double CosA = FMath::Clamp(FVector::DotProduct(Aim, To), -1.0, 1.0);
    return FMath::Acos(CosA);
}

double FAimAssist::Score(const FVector& AimOrigin, const FVector& AimDir, const FAimCandidate& Candidate,
    double ConeHalfAngleRad, double MaxRangeM)
{
    if (!Candidate.bTargetable || Candidate.Priority <= 0.0 || ConeHalfAngleRad <= 0.0 || MaxRangeM <= 0.0)
    {
        return 0.0;
    }

    const double Dist = (Candidate.Position - AimOrigin).Size();
    if (Dist > MaxRangeM)
    {
        return 0.0; // out of reach
    }

    const double Angle = AngleToTargetRad(AimOrigin, AimDir, Candidate.Position);
    if (Angle > ConeHalfAngleRad)
    {
        return 0.0; // outside the acquisition cone
    }

    const double AngleScore = 1.0 - (Angle / ConeHalfAngleRad); // 1 dead-centre, 0 at the edge
    const double RangeScore = 1.0 - (Dist / MaxRangeM);         // 1 point-blank, 0 at max range
    const double Base = AngleWeight * AngleScore + RangeWeight * RangeScore;

    return FMath::Max(0.0, Base) * Candidate.Priority;
}

int32 FAimAssist::SelectBestTarget(const FVector& AimOrigin, const FVector& AimDir,
    const TArray<FAimCandidate>& Candidates, double ConeHalfAngleRad, double MaxRangeM)
{
    int32 Best = INDEX_NONE;
    double BestScore = 0.0;

    for (int32 i = 0; i < Candidates.Num(); ++i)
    {
        const double S = Score(AimOrigin, AimDir, Candidates[i], ConeHalfAngleRad, MaxRangeM);
        if (S > BestScore) // strict: ties keep the earlier index, so selection is stable
        {
            BestScore = S;
            Best = i;
        }
    }
    return Best;
}

double FAimAssist::Magnetism(double AngleRad, double ConeHalfAngleRad)
{
    if (ConeHalfAngleRad <= 0.0 || AngleRad >= ConeHalfAngleRad)
    {
        return 0.0;
    }
    return FMath::Clamp(1.0 - (FMath::Max(0.0, AngleRad) / ConeHalfAngleRad), 0.0, 1.0);
}

FVector FAimAssist::ApplyAdhesion(const FVector& AimDir, const FVector& AimOrigin,
    const FVector& TargetPos, double Strength)
{
    const FVector Aim = AimDir.GetSafeNormal();
    const FVector To = (TargetPos - AimOrigin).GetSafeNormal();
    if (Aim.IsNearlyZero())
    {
        return AimDir;
    }
    if (To.IsNearlyZero())
    {
        return Aim;
    }

    const double S = FMath::Clamp(Strength, 0.0, 1.0);
    const FVector Blended = Aim * (1.0 - S) + To * S; // normalised lerp toward the target
    const FVector Result = Blended.GetSafeNormal();
    return Result.IsNearlyZero() ? Aim : Result; // guard a near-antiparallel blend
}
