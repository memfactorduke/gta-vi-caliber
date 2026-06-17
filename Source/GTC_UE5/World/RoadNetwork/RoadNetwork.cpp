// Copyright (c) 2026 GTC contributors

#include "RoadNetwork.h"

#include "Algo/Reverse.h"
#include "Math/UnrealMathUtility.h"

#include <limits>

// the reference Vector3.FORWARD is (0, 0, -1); used as the heading fallback for a
// zero-length segment in point_on_segment / _project_to_segment.
namespace
{
    const FVector GtcRoadNetworkForward(0.0, 0.0, -1.0);
}

int32 FRoadNetwork::RoundHalfAway(double V)
{
    // the reference roundi rounds half away from zero; FMath::RoundToInt is banker's on
    // some platforms, so do it explicitly for grid-key parity.
    return static_cast<int32>(V >= 0.0 ? FMath::FloorToDouble(V + 0.5) : FMath::CeilToDouble(V - 0.5));
}

FRoadNetwork::FRoadNetwork(double InSnap)
    : Snap(FMath::Max(0.001, InSnap))
{
}

void FRoadNetwork::AddPolyline(const TArray<FVector>& Pts)
{
    int32 Prev = -1;
    for (const FVector& P : Pts)
    {
        const int32 N = NodeFor(P);
        if (Prev != -1 && Prev != N)
        {
            AddSegment(Prev, N);
            AddSegment(N, Prev);
        }
        Prev = N;
    }
}

const TArray<int32>& FRoadNetwork::SegmentsFrom(int32 Node) const
{
    if (const TArray<int32>* Found = Adj.Find(Node))
    {
        return *Found;
    }
    static const TArray<int32> Empty;
    return Empty;
}

FRoadNetwork::FSegmentPoint FRoadNetwork::PointOnSegment(int32 Seg, double Offset) const
{
    const FVector A = Nodes[SegA[Seg]];
    const FVector B = Nodes[SegB[Seg]];
    const double Length = SegLen[Seg];
    FSegmentPoint Out;
    Out.Heading = Length > 0.0 ? (B - A).GetSafeNormal() : GtcRoadNetworkForward;
    const double T = Length <= 0.0 ? 0.0 : FMath::Clamp(Offset / Length, 0.0, 1.0);
    Out.Pos = FMath::Lerp(A, B, T);
    return Out;
}

void FRoadNetwork::BuildSpatialIndex()
{
    SegIndex.Reset();
    for (int32 Seg = 0; Seg < SegA.Num(); ++Seg)
    {
        const FVector A = Nodes[SegA[Seg]];
        const FVector B = Nodes[SegB[Seg]];
        const int32 Steps = FMath::Max(1, static_cast<int32>(FMath::CeilToDouble(SegLen[Seg] / SegCell)));
        for (int32 S = 0; S <= Steps; ++S)
        {
            const FVector P = FMath::Lerp(A, B, static_cast<double>(S) / static_cast<double>(Steps));
            const FString Key = FString::Printf(TEXT("%d_%d"),
                FMath::FloorToInt(P.X / SegCell), FMath::FloorToInt(P.Z / SegCell));
            TArray<int32>& Bucket = SegIndex.FindOrAdd(Key);
            if (Bucket.Num() == 0 || Bucket.Last() != Seg)
            {
                Bucket.Add(Seg);
            }
        }
    }
    bSegIndexBuilt = true;
}

FRoadNetwork::FNearestPoint FRoadNetwork::NearestPoint(const FVector& Pos)
{
    FNearestPoint Best;
    if (SegA.Num() == 0)
    {
        return Best; // bValid == false -> the reference empty {}
    }
    if (!bSegIndexBuilt)
    {
        BuildSpatialIndex();
    }
    const int32 Cx = FMath::FloorToInt(Pos.X / SegCell);
    const int32 Cz = FMath::FloorToInt(Pos.Z / SegCell);
    double BestDist = std::numeric_limits<double>::infinity();
    // Widen the search ring until a segment is found (a dense city hits at ring 1).
    static const int32 Rings[] = {1, 2, 4, 8, 16};
    for (const int32 Ring : Rings)
    {
        for (int32 Dz = -Ring; Dz <= Ring; ++Dz)
        {
            for (int32 Dx = -Ring; Dx <= Ring; ++Dx)
            {
                const FString Key = FString::Printf(TEXT("%d_%d"), Cx + Dx, Cz + Dz);
                if (const TArray<int32>* Bucket = SegIndex.Find(Key))
                {
                    for (const int32 Seg : *Bucket)
                    {
                        const FNearestPoint Hit = ProjectToSegment(Seg, Pos);
                        if (Hit.Dist < BestDist)
                        {
                            BestDist = Hit.Dist;
                            Best = Hit;
                        }
                    }
                }
            }
        }
        if (Best.bValid)
        {
            break;
        }
    }
    return Best;
}

TArray<int32> FRoadNetwork::FindPath(int32 Start, int32 Goal) const
{
    if (Start < 0 || Goal < 0 || Start >= Nodes.Num() || Goal >= Nodes.Num())
    {
        return TArray<int32>();
    }
    if (Start == Goal)
    {
        return TArray<int32>({Start});
    }
    const FVector GoalPos = Nodes[Goal];
    TMap<int32, int32> Came;
    TMap<int32, double> Cost;
    Cost.Add(Start, 0.0);
    // Ordered open-set: keys iterated in insertion order, matching the Godot
    // Dictionary so the strict-less tie-break is deterministic (first wins).
    TArray<int32> OpenKeys;
    TMap<int32, double> OpenScore;
    OpenKeys.Add(Start);
    OpenScore.Add(Start, FVector::Dist(Nodes[Start], GoalPos));

    int32 Guard = 0;
    while (OpenKeys.Num() > 0 && Guard < 1500)
    {
        ++Guard;
        int32 Cur = -1;
        double Best = std::numeric_limits<double>::infinity();
        for (const int32 N : OpenKeys)
        {
            if (OpenScore[N] < Best)
            {
                Best = OpenScore[N];
                Cur = N;
            }
        }
        if (Cur == Goal)
        {
            break;
        }
        OpenKeys.Remove(Cur);
        OpenScore.Remove(Cur);
        for (const int32 Seg : SegmentsFrom(Cur))
        {
            const int32 Nb = SegB[Seg];
            const double Tentative = Cost[Cur] + SegLen[Seg];
            const double* Existing = Cost.Find(Nb);
            if (Existing == nullptr || Tentative < *Existing)
            {
                Came.Add(Nb, Cur);
                Cost.Add(Nb, Tentative);
                const double Score = Tentative + FVector::Dist(Nodes[Nb], GoalPos);
                if (OpenScore.Contains(Nb))
                {
                    OpenScore[Nb] = Score;
                }
                else
                {
                    OpenKeys.Add(Nb);
                    OpenScore.Add(Nb, Score);
                }
            }
        }
    }
    if (!Came.Contains(Goal))
    {
        return TArray<int32>();
    }
    TArray<int32> Path;
    Path.Add(Goal);
    int32 C = Goal;
    while (C != Start)
    {
        C = Came[C];
        Path.Add(C);
    }
    Algo::Reverse(Path);
    return Path;
}

FRoadNetwork::FNearestPoint FRoadNetwork::ProjectToSegment(int32 Seg, const FVector& Pos) const
{
    const FVector A = Nodes[SegA[Seg]];
    const FVector B = Nodes[SegB[Seg]];
    const FVector2D Ax(A.X, A.Z);
    const FVector2D Ab(B.X - A.X, B.Z - A.Z);
    const double Len2 = Ab.SizeSquared();
    const double T = Len2 <= 0.0
        ? 0.0
        : FMath::Clamp((FVector2D(Pos.X, Pos.Z) - Ax).Dot(Ab) / Len2, 0.0, 1.0);
    const FVector2D Closest = Ax + Ab * T;
    FNearestPoint Out;
    Out.bValid = true;
    Out.Seg = Seg;
    Out.Offset = T * SegLen[Seg];
    Out.Pos = FVector(Closest.X, A.Y + (B.Y - A.Y) * T, Closest.Y);
    Out.Heading = SegLen[Seg] > 0.0 ? (B - A).GetSafeNormal() : GtcRoadNetworkForward;
    Out.Dist = FVector2D::Distance(FVector2D(Pos.X, Pos.Z), Closest);
    return Out;
}

int32 FRoadNetwork::NodeFor(const FVector& P)
{
    const FString Key = FString::Printf(TEXT("%d_%d"),
        RoundHalfAway(P.X / Snap), RoundHalfAway(P.Z / Snap));
    if (const int32* Found = Index.Find(Key))
    {
        return *Found;
    }
    const int32 I = Nodes.Num();
    Nodes.Add(P);
    Index.Add(Key, I);
    Adj.Add(I, TArray<int32>());
    return I;
}

void FRoadNetwork::AddSegment(int32 A, int32 B)
{
    if (A == B)
    {
        return;
    }
    const int32 I = SegA.Num();
    SegA.Add(A);
    SegB.Add(B);
    SegLen.Add(FVector::Dist(Nodes[A], Nodes[B]));
    Adj.FindOrAdd(A).Add(I);
}
