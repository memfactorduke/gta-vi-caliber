// Copyright (c) 2026 GTC contributors

#include "GpsNavigation.h"

#include "Math/UnrealMathUtility.h"

#include <limits>

FVector FGpsNavigation::Ground(const FVector& V)
{
    return FVector(V.X, 0.0, V.Z);
}

double FGpsNavigation::RouteLength(const TArray<FVector>& Route)
{
    double Total = 0.0;
    for (int32 i = 0; i < Route.Num() - 1; ++i)
    {
        Total += Ground(Route[i + 1] - Route[i]).Size();
    }
    return Total;
}

int32 FGpsNavigation::NearestSegment(const FVector& Pos, const TArray<FVector>& Route)
{
    if (Route.Num() < 2)
    {
        return 0;
    }
    const FVector Flat = Ground(Pos);
    int32 BestI = 0;
    double BestD = std::numeric_limits<double>::infinity();
    for (int32 i = 0; i < Route.Num() - 1; ++i)
    {
        const FVector Proj = ProjectPoint(Flat, Route[i], Route[i + 1]);
        const double D = FVector::DistSquared(Flat, Proj);
        if (D < BestD)
        {
            BestD = D;
            BestI = i;
        }
    }
    return BestI;
}

double FGpsNavigation::DistanceRemaining(const FVector& Pos, const TArray<FVector>& Route)
{
    if (Route.Num() < 2)
    {
        return 0.0;
    }
    const FVector Flat = Ground(Pos);
    const int32 Seg = NearestSegment(Flat, Route);
    const FVector A = Route[Seg];
    const FVector B = Route[Seg + 1];
    const double T = SegmentT(Flat, A, B);
    double Remaining = Ground(B - A).Size() * (1.0 - T);
    for (int32 i = Seg + 1; i < Route.Num() - 1; ++i)
    {
        Remaining += Ground(Route[i + 1] - Route[i]).Size();
    }
    return Remaining;
}

double FGpsNavigation::Progress(const FVector& Pos, const TArray<FVector>& Route)
{
    const double Total = RouteLength(Route);
    if (Total < Eps)
    {
        return 1.0;
    }
    return FMath::Clamp(1.0 - DistanceRemaining(Pos, Route) / Total, 0.0, 1.0);
}

double FGpsNavigation::EtaSeconds(const FVector& Pos, const TArray<FVector>& Route, double Speed)
{
    if (Speed <= Eps)
    {
        // A stopped player has no finite ETA.
        return std::numeric_limits<double>::infinity();
    }
    return DistanceRemaining(Pos, Route) / Speed;
}

FGpsNavigation::FNextTurn FGpsNavigation::NextTurn(const FVector& Pos, const TArray<FVector>& Route, double TurnThresholdRadians)
{
    FNextTurn Result;
    if (Route.Num() < 3)
    {
        return Result;
    }
    const FVector Flat = Ground(Pos);
    const int32 Seg = NearestSegment(Flat, Route);
    // Examine each interior waypoint ahead of the player's current segment.
    for (int32 i = Seg + 1; i < Route.Num() - 1; ++i)
    {
        const FVector Incoming = Ground(Route[i] - Route[i - 1]);
        const FVector Outgoing = Ground(Route[i + 1] - Route[i]);
        if (Incoming.Size() < Eps || Outgoing.Size() < Eps)
        {
            continue;
        }
        const double Signed = SignedTurn(Incoming, Outgoing);
        if (FMath::Abs(Signed) <= TurnThresholdRadians)
        {
            continue;
        }
        Result.bHasTurn = true;
        Result.Position = Route[i];
        Result.Distance = DistanceToWaypoint(Flat, Route, Seg, i);
        Result.Direction = Signed > 0.0 ? ETurnDirection::Left : ETurnDirection::Right;
        return Result;
    }
    return Result;
}

bool FGpsNavigation::HasArrived(const FVector& Pos, const TArray<FVector>& Route, double ArriveRadius)
{
    if (Route.Num() == 0)
    {
        return false;
    }
    const FVector Dest = Route[Route.Num() - 1];
    return Ground(Dest - Pos).Size() <= ArriveRadius;
}

FVector FGpsNavigation::DirectionToNext(const FVector& Pos, const TArray<FVector>& Route)
{
    if (Route.Num() < 2)
    {
        return FVector::ZeroVector;
    }
    const FVector Flat = Ground(Pos);
    const int32 Seg = NearestSegment(Flat, Route);
    const FVector A = Route[Seg];
    const FVector B = Route[Seg + 1];
    const FVector SegDir = Ground(B - A);
    if (SegDir.Size() < Eps)
    {
        return FVector::ZeroVector;
    }
    return SegDir.GetSafeNormal();
}

// --- helpers ----------------------------------------------------------------

double FGpsNavigation::SegmentT(const FVector& P, const FVector& A, const FVector& B)
{
    const FVector AB = Ground(B - A);
    const double LenSq = AB.SizeSquared();
    if (LenSq < Eps)
    {
        return 0.0;
    }
    return FMath::Clamp(FVector::DotProduct(Ground(P - A), AB) / LenSq, 0.0, 1.0);
}

FVector FGpsNavigation::ProjectPoint(const FVector& P, const FVector& A, const FVector& B)
{
    const double T = SegmentT(P, A, B);
    return Ground(A) + Ground(B - A) * T;
}

double FGpsNavigation::SignedTurn(const FVector& Incoming, const FVector& Outgoing)
{
    const FVector A = Ground(Incoming);
    const FVector B = Ground(Outgoing);
    if (A.Size() < Eps || B.Size() < Eps)
    {
        return 0.0;
    }
    // Cross product's up component: a x b about the up axis. Positive turns left.
    const double Cross = A.Z * B.X - A.X * B.Z;
    const double Dot = A.X * B.X + A.Z * B.Z;
    return FMath::Atan2(Cross, Dot);
}

double FGpsNavigation::DistanceToWaypoint(const FVector& Pos, const TArray<FVector>& Route, int32 Seg, int32 Target)
{
    const FVector A = Route[Seg];
    const FVector B = Route[Seg + 1];
    const double T = SegmentT(Pos, A, B);
    double Dist = Ground(B - A).Size() * (1.0 - T);
    for (int32 i = Seg + 1; i < Target; ++i)
    {
        Dist += Ground(Route[i + 1] - Route[i]).Size();
    }
    return Dist;
}
