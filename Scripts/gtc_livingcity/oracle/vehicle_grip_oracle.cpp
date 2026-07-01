// Out-of-tree oracle for FVehicleGripResolver — compiles + runs the REAL
// VehicleGripResolver.cpp against the REAL RoadGrip.cpp + VehicleHandling.cpp and
// re-asserts the grip chain the live adapter (UGTCVehicleHandlingComponent) leans on:
// surface wetness + ground speed -> FRoadGrip -> x tyre grip -> FVehicleHandling slip/
// drift/lateral retention. This is the pure half the UObject shell only plumbs.
#include "../../../Source/GTC_UE5/Vehicles/Handling/VehicleGripResolver.h"
#include "harness.h"

static const double KmhToCms = 1.0 / FVehicleGripResolver::CmsToKmh;

static FVehicleGripResolver::FInput MakeIn(double Wetness, double GroundKmh, double TyreGrip, bool bHandbrake)
{
    FVehicleGripResolver::FInput In;
    In.Velocity = FVector(1, 0, 0) * (GroundKmh * KmhToCms); // tracking straight ahead
    In.Forward = FVector(1, 0, 0);
    In.Right = FVector(0, 1, 0);
    In.Wetness = Wetness;
    In.TyreGrip = TyreGrip;
    In.bHandbrake = bHandbrake;
    return In;
}

int main()
{
    const FRoadGrip::FParams P; // WetGripLoss 0.35, aquaplane 90..160 km/h, floor 0.25
    const FVector Fwd(1, 0, 0);
    const FVector Right(0, 1, 0);

    // Dry road is the identity: grip = tyre grip, nothing slides, no aquaplane.
    {
        const auto O = FVehicleGripResolver::Resolve(MakeIn(0.0, 72.0, 1.0, false), P);
        CheckNear(O.RoadGrip, 1.0, "dry road -> full road grip");
        CheckNear(O.GripFactor, 1.0, "dry + fresh tyres -> full grip");
        Check(!O.bAquaplaning, "dry road never aquaplanes");
        CheckNear(O.DriftFactor, 0.0, "tracking straight -> no drift");
        CheckNear(O.GroundSpeedKmh, 72.0, "ground speed reported in km/h", 1e-3);
    }

    const auto WetSlow = FVehicleGripResolver::Resolve(MakeIn(1.0, 50.0, 1.0, false), P);
    CheckNear(WetSlow.RoadGrip, 0.65, "wet-slow road grip == 1 - WetGripLoss (0.65)");
    Check(!WetSlow.bAquaplaning, "wet but under aquaplane speed -> not aquaplaning");

    const auto WetFast = FVehicleGripResolver::Resolve(MakeIn(1.0, 200.0, 1.0, false), P);
    const auto DryFast = FVehicleGripResolver::Resolve(MakeIn(0.0, 200.0, 1.0, false), P);
    Check(WetFast.bAquaplaning, "wet + fast aquaplanes");
    CheckNear(WetFast.RoadGrip, 0.25, "fully-aquaplaning road grip == AquaplaneFloor (0.25)");
    Check(WetFast.LateralRetention > DryFast.LateralRetention, "aquaplaning slides more than a dry road");

    // Worn tyres compound with the road; the road's own grip is unchanged.
    const auto Worn = FVehicleGripResolver::Resolve(MakeIn(1.0, 50.0, 0.6, false), P);
    Check(Worn.GripFactor < WetSlow.GripFactor, "worn tyres cut grip below fresh on the same road");
    CheckNear(Worn.RoadGrip, WetSlow.RoadGrip, "tyre wear does not change the road's grip");
    CheckNear(Worn.GripFactor, 0.65 * 0.6, "grip factor == road grip x tyre grip");

    // Lateral retention hits its documented endpoints.
    const auto Grip = FVehicleGripResolver::Resolve(MakeIn(0.0, 72.0, 1.0, false), P);
    const auto Hand = FVehicleGripResolver::Resolve(MakeIn(0.0, 72.0, 1.0, true), P);
    CheckNear(Grip.LateralRetention, 0.10, "full grip, gripping -> GripRetention (0.10)");
    CheckNear(Hand.LateralRetention, 0.85, "full grip, handbrake -> HandbrakeRetention (0.85)");

    // A real forward slide: heading forward but moving partly sideways.
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = FVector(1000, 600, 0);
        In.Forward = Fwd;
        In.Right = Right;
        In.Wetness = 0.0;
        In.TyreGrip = 1.0;
        const auto O = FVehicleGripResolver::Resolve(In, P);
        Check(O.SlipAngleRad > 1e-4, "sliding sideways -> positive slip angle");
        Check(O.DriftFactor > 0.0, "sliding forward at speed -> drifting");
        CheckNear(FVector::DotProduct(O.GrippedVelocity, Fwd), 1000.0, "gripped velocity preserves forward speed", 1.0);
        Check(FVector::DotProduct(O.GrippedVelocity, Right) < 600.0, "gripped velocity kills most sideways slide");
    }

    // Reversing straight is NOT a drift (regression: unsigned slip would read pi -> full drift).
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = FVector(-1500, 0, 0);
        In.Forward = Fwd;
        In.Right = Right;
        const auto O = FVehicleGripResolver::Resolve(In, P);
        CheckNear(O.DriftFactor, 0.0, "reversing straight scores no drift");
    }

    // Airborne / falling: vertical drop is not road speed -> no phantom aquaplane/drift.
    {
        FVehicleGripResolver::FInput In;
        In.Velocity = FVector(0, 0, -6000);
        In.Forward = Fwd;
        In.Right = Right;
        In.Wetness = 1.0;
        In.TyreGrip = 1.0;
        const auto O = FVehicleGripResolver::Resolve(In, P);
        CheckNear(O.GroundSpeedKmh, 0.0, "falling straight down -> zero ground speed", 1e-3);
        Check(!O.bAquaplaning, "a falling car does not aquaplane");
        CheckNear(O.DriftFactor, 0.0, "a falling car does not drift");
    }

    // Parked car: degenerate-safe, dry grip full.
    {
        FVehicleGripResolver::FInput In;
        In.Forward = Fwd;
        In.Right = Right;
        const auto O = FVehicleGripResolver::Resolve(In, P);
        CheckNear(O.SlipAngleRad, 0.0, "parked -> no slip");
        CheckNear(O.DriftFactor, 0.0, "parked -> no drift");
        CheckNear(O.GripFactor, 1.0, "parked on a dry road -> full grip");
    }

    return OracleSummary("vehicle_grip_oracle");
}
