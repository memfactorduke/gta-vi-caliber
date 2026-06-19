// Copyright Epic Games, Inc. All Rights Reserved.

#include "NpcLocomotion.h"

#include "../Steering/NpcSteering.h"
#include "Math/UnrealMathUtility.h"

double FNpcLocomotion::GroundDistance(const FVector& A, const FVector& B)
{
    // Planar in UE ground (X/Y); the math plane's distance is the same magnitude.
    return FVector2D(A.X - B.X, A.Y - B.Y).Size();
}

bool FNpcLocomotion::HasArrived(const FVector& Pos, const FVector& Goal, double Tolerance)
{
    return GroundDistance(Pos, Goal) <= Tolerance;
}

FVector FNpcLocomotion::FleeGoal(const FVector& SelfPos, const FVector& ThreatPos, double Distance)
{
    // Run directly away from the threat in the math plane, then plant a goal that
    // far ahead and map back to UE ground. FromPlane zeroes Z, so the flee goal
    // sits on the NPC's own ground height by construction.
    const FVector Away = FNpcBrain::FleeDir(ToPlane(SelfPos), ToPlane(ThreatPos));
    if (Away.IsNearlyZero(Eps))
    {
        // Threat is on top of us — pick a stable arbitrary direction (UE +X).
        return SelfPos + FVector(Distance, 0.0, 0.0);
    }
    return SelfPos + FromPlane(Away) * Distance;
}

FVector FNpcLocomotion::DesiredVelocity(
    FNpcBrain::EState State,
    const FVector& SelfPos,
    const FVector& Goal,
    const TArray<FVector>& Neighbors,
    const TArray<FPedestrianTraffic::FCar>& Cars,
    double WalkSpeed,
    double RunSpeed,
    const FNpcLocomotionParams& Params)
{
    const double Speed = FNpcBrain::SpeedFor(State, WalkSpeed, RunSpeed);
    if (Speed <= Eps)
    {
        // Idle, or a degenerate speed: stand still.
        return FVector::ZeroVector;
    }

    // Everything below is in the Godot XZ math plane.
    const FVector P = ToPlane(SelfPos);
    const FVector G = ToPlane(Goal);

    TArray<FVector> PlaneNeighbors;
    PlaneNeighbors.Reserve(Neighbors.Num());
    for (const FVector& N : Neighbors)
    {
        PlaneNeighbors.Add(ToPlane(N));
    }

    // Goal term: Flee just sprints away (Seek, no easing — you don't decelerate
    // into the point you're fleeing toward); a real destination eases in (Arrive).
    const FVector GoalVel = (State == FNpcBrain::EState::Flee)
        ? FNpcSteering::Seek(P, G, Speed)
        : FNpcSteering::Arrive(P, G, Speed, Params.SlowRadius, Params.ArriveRadius);

    // FNpcSteering::Separation is parity-locked to the Godot metres frame, where it
    // saturates the crowd push at the walk speed (limit_length on a ~1.5 m/s scale).
    // At the cm/s adapter scale a raw push of magnitude ~1 vanishes against a ~150
    // goal velocity — so the crowd, and the player give-way that rides this term,
    // would clip straight through. Restore the faithful balance: saturate the push
    // to a unit direction, then scale it onto the same cm/s scale as the goal term.
    const FVector SepVel =
        FNpcSteering::Separation(P, PlaneNeighbors, Params.SeparationRadius, 1.0) * Speed;

    // Traffic reflex: dodge the most threatening closing car, weighted by how
    // imminent the broadside is. Skipped while fleeing — a fleeing pedestrian is
    // already running for their life and shouldn't second-guess the curb.
    FVector DodgeVel = FVector::ZeroVector;
    double DodgeW = 0.0;
    if (State != FNpcBrain::EState::Flee && Cars.Num() > 0)
    {
        TArray<FPedestrianTraffic::FCar> PlaneCars;
        PlaneCars.Reserve(Cars.Num());
        for (const FPedestrianTraffic::FCar& C : Cars)
        {
            FPedestrianTraffic::FCar PC;
            PC.Pos = ToPlane(C.Pos);
            PC.Vel = ToPlane(C.Vel);
            PlaneCars.Add(PC);
        }

        const FPedestrianTraffic::FThreat Threat = FPedestrianTraffic::NearestThreat(
            P, GoalVel, PlaneCars, Params.CarReactRadius, Params.CarHorizonSec);

        if (Threat.Threat >= Params.DodgeThreshold)
        {
            DodgeVel = FPedestrianTraffic::DodgeVelocity(P, Threat.Pos, Threat.Vel, Speed);
            DodgeW = Params.DodgeWeight * Threat.Threat;
        }
    }

    const TArray<FVector> Vectors = {GoalVel, SepVel, DodgeVel};
    const TArray<double> Weights = {Params.GoalWeight, Params.SeparationWeight, DodgeW};

    const FVector Combined = FNpcSteering::Combine(Vectors, Weights, Speed);
    return FromPlane(Combined);
}
