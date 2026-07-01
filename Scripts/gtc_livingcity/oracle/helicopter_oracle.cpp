// Out-of-tree oracle for FHelicopterFlightResolver — compiles + runs the REAL
// HelicopterFlightResolver.cpp against the REAL HelicopterFlight.cpp and exercises the adapter the
// live UGTCHelicopterComponent runs: the four rotor controls -> a world velocity + heading (the
// core), integrated into a world position with a ground floor + HUD telemetry (the resolver).
#include "../../../Source/GTC_UE5/Vehicles/Aircraft/HelicopterFlightResolver.h"
#include "../../../Source/GTC_UE5/Vehicles/Aircraft/HelicopterFlight.h"
#include "harness.h"

int main()
{
    // --- Position integration + ground floor + telemetry. ---
    {
        FHelicopterFlightResolver::FInput In;
        In.Position = FVector(0, 0, 1000);
        In.Velocity = FVector(100, 0, 50);
        In.GroundZ = 0.0;
        In.Dt = 1.0;
        const auto O = FHelicopterFlightResolver::Integrate(In);
        Check(O.Position.X == 100.0 && O.Position.Z == 1050.0, "position advances by velocity * dt");
        Check(!O.bGrounded, "high above the floor -> airborne");
        CheckNear(O.AltitudeAgl, 1050.0, "altitude AGL is height over the ground");
        CheckNear(O.ClimbRateCmS, 50.0, "climb rate is vertical velocity");
        CheckNear(O.GroundSpeedCmS, 100.0, "ground speed is the horizontal velocity");
    }
    {
        // Descending into the deck: the chopper is clamped to the ground and reads grounded.
        FHelicopterFlightResolver::FInput In;
        In.Position = FVector(0, 0, 10);
        In.Velocity = FVector(0, 0, -100);
        In.GroundZ = 0.0;
        In.Dt = 1.0;
        const auto O = FHelicopterFlightResolver::Integrate(In);
        CheckNear(O.Position.Z, 0.0, "cannot sink through the ground");
        Check(O.bGrounded, "on the deck -> grounded");
        CheckNear(O.ClimbRateCmS, 0.0, "grounded -> no reported descent");
        CheckNear(O.AltitudeAgl, 0.0, "grounded -> zero altitude");
    }
    {
        // Ground speed drops the vertical component (3-4-5).
        FHelicopterFlightResolver::FInput In;
        In.Velocity = FVector(30, 40, 9999);
        In.Position = FVector(0, 0, 100000);
        const auto O = FHelicopterFlightResolver::Integrate(In);
        CheckNear(O.GroundSpeedCmS, 50.0, "ground speed ignores vertical velocity");
    }

    // --- Flight envelope through the real rotor model. ---
    FHelicopterFlight::FParams P; // Gravity 980, MaxLift 2000 -> hover at 0.49
    {
        FHelicopterFlight H;
        H.Configure(P);
        CheckNear(H.HoverCollective(), 980.0 / 2000.0, "hover collective cancels gravity (0.49)");
    }
    {
        // Hover collective holds altitude: vertical velocity stays ~0.
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int i = 0; i < 20; ++i)
        {
            H.Update(Hover, 0.0, 0.0, 0.0, 0.1);
        }
        Check(H.Velocity().Z < 1.0 && H.Velocity().Z > -1.0, "at hover collective the chopper holds altitude");
    }
    {
        // Full collective climbs; no collective sinks.
        FHelicopterFlight Up;
        Up.Configure(P);
        FHelicopterFlight Down;
        Down.Configure(P);
        for (int i = 0; i < 10; ++i)
        {
            Up.Update(1.0, 0.0, 0.0, 0.0, 0.1);
            Down.Update(0.0, 0.0, 0.0, 0.0, 0.1);
        }
        Check(Up.Velocity().Z > 0.0, "full collective climbs");
        Check(Down.Velocity().Z < 0.0, "no collective sinks");
    }
    {
        // Nose down (negative cyclic pitch) accelerates forward.
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int i = 0; i < 10; ++i)
        {
            H.Update(Hover, -1.0, 0.0, 0.0, 0.1);
        }
        FHelicopterFlightResolver::FInput In;
        In.Velocity = H.Velocity();
        const auto O = FHelicopterFlightResolver::Integrate(In);
        Check(O.GroundSpeedCmS > 0.0, "nose-down cyclic builds forward ground speed");
    }
    {
        // The pedal turns the heading.
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int i = 0; i < 10; ++i)
        {
            H.Update(Hover, 0.0, 0.0, 1.0, 0.1);
        }
        Check(H.Heading() > 0.0, "full pedal turns the heading");
    }
    {
        // The core's Right basis is Godot-handed: positive roll drives -Y at heading 0, which is the
        // actor's LEFT in UE. The component negates roll (feeds -roll) so "bank right" slides right;
        // this pins the convention the component relies on.
        FHelicopterFlight H;
        H.Configure(P);
        const double Hover = H.HoverCollective();
        for (int i = 0; i < 5; ++i)
        {
            H.Update(Hover, 0.0, 1.0, 0.0, 0.1); // raw roll +1 into the core
        }
        Check(H.Velocity().Y < 0.0, "core convention: positive roll drives -Y (adapter negates it for UE)");
    }

    return OracleSummary("helicopter_oracle");
}
