// Out-of-tree oracle for FFixedWingFlightResolver — compiles + runs the REAL
// FixedWingFlightResolver.cpp against the REAL FixedWingFlight.cpp and exercises the adapter the live
// UGTCFixedWingComponent runs: throttle/elevator/aileron -> a world velocity + heading (the core),
// integrated into a world position with a ground floor + stall/altitude telemetry (the resolver).
#include "../../../Source/GTC_UE5/Vehicles/Aircraft/FixedWingFlightResolver.h"
#include "../../../Source/GTC_UE5/Vehicles/Aircraft/FixedWingFlight.h"
#include "harness.h"

int main()
{
    // --- Position integration + ground floor + stall telemetry. ---
    {
        FFixedWingFlightResolver::FInput In;
        In.Position = FVector(0, 0, 1000);
        In.Velocity = FVector(1500, 0, 100);
        In.Dt = 1.0;
        In.Airspeed = 1500.0;
        In.StallSpeed = 800.0;
        const auto O = FFixedWingFlightResolver::Resolve(In);
        Check(O.Position.X == 1500.0 && O.Position.Z == 1100.0, "position advances by velocity * dt");
        Check(!O.bGrounded, "high above the floor -> airborne");
        CheckNear(O.AltitudeAgl, 1100.0, "altitude AGL is height over the ground");
        CheckNear(O.ClimbRateCmS, 100.0, "climb rate is vertical velocity");
        CheckNear(O.GroundSpeedCmS, 1500.0, "ground speed is the horizontal velocity");
        Check(!O.bStalled, "airspeed above stall -> flying");
        CheckNear(O.StallMargin, (1500.0 - 800.0) / 800.0, "stall margin is the airspeed cushion over stall");
    }
    {
        // Below stall: flagged, zero margin.
        FFixedWingFlightResolver::FInput In;
        In.Velocity = FVector(500, 0, -600);
        In.Airspeed = 500.0;
        In.StallSpeed = 800.0;
        In.Position = FVector(0, 0, 100000);
        const auto O = FFixedWingFlightResolver::Resolve(In);
        Check(O.bStalled, "airspeed below stall -> stalled");
        CheckNear(O.StallMargin, 0.0, "below stall -> zero margin");
    }
    {
        // Sitting on the runway.
        FFixedWingFlightResolver::FInput In;
        In.Position = FVector(0, 0, 5);
        In.Velocity = FVector(0, 0, -100);
        In.Dt = 1.0;
        In.StallSpeed = 800.0;
        const auto O = FFixedWingFlightResolver::Resolve(In);
        CheckNear(O.Position.Z, 0.0, "cannot sink through the runway");
        Check(O.bGrounded, "on the runway -> grounded");
        CheckNear(O.ClimbRateCmS, 0.0, "grounded -> no reported descent");
    }
    {
        // Stall margin saturates at 1 well above stall.
        FFixedWingFlightResolver::FInput In;
        In.Airspeed = 1600.0;
        In.StallSpeed = 800.0;
        In.Position = FVector(0, 0, 100000);
        const auto O = FFixedWingFlightResolver::Resolve(In);
        CheckNear(O.StallMargin, 1.0, "margin saturates at 1 (2x stall speed)");
    }

    // --- Flight envelope through the real model. ---
    FFixedWingFlight::FParams P; // MaxThrust 500 / Drag 0.2 -> terminal cruise 2500; stall 800
    {
        // Full throttle builds airspeed from a standstill up past stall.
        FFixedWingFlight F;
        F.Configure(P);
        Check(F.IsStalled(), "starts stalled at rest (airspeed 0)");
        for (int i = 0; i < 200; ++i)
        {
            F.Update(1.0, 0.0, 0.0, 0.1);
        }
        Check(F.Airspeed() > P.StallSpeed, "full throttle accelerates past stall speed");
        Check(!F.IsStalled(), "once fast enough the wings fly");
    }
    {
        // Stalled and unpowered, the plane sinks.
        FFixedWingFlight F;
        F.Configure(P);
        F.Update(0.0, 0.0, 0.0, 0.1);
        Check(F.ClimbRate() < 0.0, "stalled -> sinks");
        Check(F.Velocity().Z < 0.0, "world velocity Z is the sink rate");
    }
    {
        // Climbing costs airspeed: pulling the nose up bleeds speed vs holding level.
        FFixedWingFlight Level;
        Level.Configure(P);
        FFixedWingFlight Climb;
        Climb.Configure(P);
        for (int i = 0; i < 120; ++i) // both reach cruising airspeed at full throttle
        {
            Level.Update(1.0, 0.0, 0.0, 0.1);
            Climb.Update(1.0, 0.0, 0.0, 0.1);
        }
        for (int i = 0; i < 20; ++i) // now one pulls up
        {
            Level.Update(1.0, 0.0, 0.0, 0.1);
            Climb.Update(1.0, 1.0, 0.0, 0.1);
        }
        Check(Climb.ClimbRate() > 0.0, "up-elevator climbs");
        Check(Climb.Airspeed() < Level.Airspeed(), "climbing costs airspeed");
    }
    {
        // Aileron turns the heading; increasing heading is a right turn in UE (+X -> +Y).
        FFixedWingFlight F;
        F.Configure(P);
        for (int i = 0; i < 120; ++i) // get flying speed for control authority
        {
            F.Update(1.0, 0.0, 0.0, 0.1);
        }
        const double H0 = F.Heading();
        for (int i = 0; i < 20; ++i)
        {
            F.Update(1.0, 0.0, 1.0, 0.1);
        }
        Check(F.Heading() > H0, "positive aileron turns the heading right (increasing yaw)");
    }
    {
        // Stall recovery: a stalled plane that spools up its airspeed flies again.
        FFixedWingFlight F;
        F.Configure(P);
        F.Update(0.0, 0.0, 0.0, 0.1);
        Check(F.IsStalled(), "unpowered -> stalled");
        for (int i = 0; i < 200; ++i)
        {
            F.Update(1.0, 0.0, 0.0, 0.1);
        }
        Check(!F.IsStalled(), "throttle recovers from the stall");
    }

    return OracleSummary("fixedwing_oracle");
}
