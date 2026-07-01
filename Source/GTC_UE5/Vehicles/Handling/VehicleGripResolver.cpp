// Copyright Epic Games, Inc. All Rights Reserved.

#include "VehicleGripResolver.h"

#include "Math/UnrealMathUtility.h"

FVehicleGripResolver::FOutput FVehicleGripResolver::Resolve(const FInput& In, const FRoadGrip::FParams& RoadParams)
{
    FOutput Out;

    // Ground-plane velocity: the part of the velocity in the car's Forward/Right plane. Dropping
    // whatever is out of that plane (a ramp jump / fall's vertical drop) keeps an airborne car
    // from counting its plummet as road speed and spuriously aquaplaning or "drifting".
    const FVector Fwd = In.Forward.GetSafeNormal();
    const FVector Rgt = In.Right.GetSafeNormal();
    const double FwdSpeed = FVector::DotProduct(In.Velocity, Fwd); // signed: <0 means reversing
    const double RgtSpeed = FVector::DotProduct(In.Velocity, Rgt);
    const FVector GroundVel = Fwd * FwdSpeed + Rgt * RgtSpeed;
    const double GroundSpeed = GroundVel.Size();
    Out.GroundSpeedKmh = GroundSpeed * CmsToKmh;

    // 1) The road's grip from the wetness + ground speed, then fold the tyres' own grip in.
    //    Kept separate on purpose: FRoadGrip is the ROAD's contribution, TyreGrip is the car's
    //    (wear). Their product is the single GripFactor handling consumes.
    Out.RoadGrip = FRoadGrip::GripFactor(In.Wetness, Out.GroundSpeedKmh, RoadParams);
    const double Tyre = FMath::Clamp(In.TyreGrip, 0.0, 1.0);
    Out.GripFactor = FMath::Clamp(Out.RoadGrip * Tyre, 0.0, 1.0);

    // 2) The aquaplaning tell — drives the loss-of-control FX + a "slow down, it's flooding" cue.
    Out.AquaplaneAmount = FRoadGrip::AquaplaneAmount(In.Wetness, Out.GroundSpeedKmh, RoadParams);
    Out.bAquaplaning = FRoadGrip::IsAquaplaning(In.Wetness, Out.GroundSpeedKmh, RoadParams);

    // 3) Handling: how sideways is the car, and how much of that slide does it keep? Worse grip
    //    (a wet road, worn tyres, or the handbrake) lets the tail step further out. Slip/drift are
    //    measured on the GROUND velocity, and a car driving straight in REVERSE (FwdSpeed < 0,
    //    slip -> pi) is not a stunt drift — gate it out so the scorer + FX don't fire on reverse.
    Out.SlipAngleRad = FVehicleHandling::SlipAngleRad(Fwd, GroundVel);
    Out.DriftFactor = (FwdSpeed < 0.0) ? 0.0 : FVehicleHandling::DriftFactor(Out.SlipAngleRad, GroundSpeed);
    Out.LateralRetention = FVehicleHandling::LateralRetention(In.bHandbrake, Out.GripFactor);

    // 4) The gripped / sliding velocity the chassis should adopt this frame. ApplyGrip works on the
    //    FULL velocity so the out-of-plane (vertical) part is preserved — only the sideways ground
    //    component is scaled by how much grip is left.
    Out.GrippedVelocity = FVehicleHandling::ApplyGrip(In.Velocity, Fwd, Rgt, Out.LateralRetention);

    return Out;
}
