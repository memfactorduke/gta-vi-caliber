// Copyright Epic Games, Inc. All Rights Reserved.

#include "TurnChoice.h"

#include "Math/UnrealMathUtility.h"

namespace
{
    // Planar (XZ) unit heading, or zero for a degenerate vector.
    FVector PlanarUnit(const FVector& V)
    {
        const FVector H(V.X, 0.0, V.Z);
        return H.GetSafeNormal();
    }
}

double FTurnChoice::SignedTurn(const FVector& Incoming, const FVector& Outgoing)
{
    const FVector In = PlanarUnit(Incoming);
    const FVector Out = PlanarUnit(Outgoing);
    if (In == FVector::ZeroVector || Out == FVector::ZeroVector)
    {
        return 0.0;
    }
    const double Dot = In.X * Out.X + In.Z * Out.Z;
    // XZ "cross" (the Y component): positive = left turn, matching FGpsNavigation.
    const double Perp = In.X * Out.Z - In.Z * Out.X;
    return FMath::Atan2(Perp, Dot);
}

FTurnChoice::ETurn FTurnChoice::Classify(
    const FVector& Incoming, const FVector& Outgoing, double StraightThreshold, double UTurnThreshold)
{
    const double Angle = SignedTurn(Incoming, Outgoing);
    const double Mag = FMath::Abs(Angle);
    if (Mag <= StraightThreshold)
    {
        return ETurn::Straight;
    }
    if (Mag >= UTurnThreshold)
    {
        return ETurn::UTurn;
    }
    return Angle > 0.0 ? ETurn::Left : ETurn::Right;
}

int32 FTurnChoice::ChooseByRoute(const TArray<int32>& OutgoingNodeIds, int32 DesiredNextNode)
{
    for (int32 I = 0; I < OutgoingNodeIds.Num(); ++I)
    {
        if (OutgoingNodeIds[I] == DesiredNextNode)
        {
            return I;
        }
    }
    return -1;
}

int32 FTurnChoice::ChooseStraightest(
    const FVector& Incoming, const TArray<FVector>& OutgoingHeadings, bool bAllowUTurn, double UTurnThreshold)
{
    const FVector In = PlanarUnit(Incoming);
    if (In == FVector::ZeroVector)
    {
        return -1;
    }
    int32 Best = -1;
    double BestDot = -2.0; // below any real cosine
    for (int32 I = 0; I < OutgoingHeadings.Num(); ++I)
    {
        const FVector Out = PlanarUnit(OutgoingHeadings[I]);
        if (Out == FVector::ZeroVector)
        {
            continue;
        }
        if (!bAllowUTurn && FMath::Abs(SignedTurn(In, Out)) >= UTurnThreshold)
        {
            continue;
        }
        const double Dot = In.X * Out.X + In.Z * Out.Z;
        if (Dot > BestDot) // strict > keeps the lower index on a tie
        {
            BestDot = Dot;
            Best = I;
        }
    }
    return Best;
}
