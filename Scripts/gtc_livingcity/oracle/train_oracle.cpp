// Out-of-tree oracle for FRailPathResolver — compiles + runs the REAL RailPathResolver.cpp against the
// REAL TrainMover.cpp and exercises the adapter the live UGTCTrainComponent runs: the 1-DOF controller
// (distance + speed, braking to a stop at each station) mapped onto a world transform along a looping
// rail polyline.
#include "../../../Source/GTC_UE5/Vehicles/Rail/RailPathResolver.h"
#include "../../../Source/GTC_UE5/Vehicles/Rail/TrainMover.h"
#include "harness.h"

int main()
{
    // A 1000-cm square loop: perimeter 4000.
    TArray<FVector> Square;
    Square.Add(FVector(0, 0, 0));
    Square.Add(FVector(1000, 0, 0));
    Square.Add(FVector(1000, 1000, 0));
    Square.Add(FVector(0, 1000, 0));

    // --- Rail path sampling. ---
    CheckNear(FRailPathResolver::LoopLength(Square), 4000.0, "loop length sums every segment + the closing one");
    {
        const auto S = FRailPathResolver::Sample(Square, 0.0);
        Check(S.bValid, "start of the loop is valid");
        Check(S.Position.X == 0.0 && S.Position.Y == 0.0, "distance 0 is the first point");
        CheckNear(S.Forward.X, 1.0, "forward points along the first segment (+X)");
    }
    {
        const auto S = FRailPathResolver::Sample(Square, 500.0);
        Check(S.Position.X == 500.0 && S.Position.Y == 0.0, "halfway along the first side");
    }
    {
        const auto S = FRailPathResolver::Sample(Square, 1500.0);
        Check(S.Position.X == 1000.0 && S.Position.Y == 500.0, "into the second side");
        CheckNear(S.Forward.Y, 1.0, "forward turns to +Y on the second side");
    }
    {
        const auto S = FRailPathResolver::Sample(Square, 4500.0);
        Check(S.Position.X == 500.0 && S.Position.Y == 0.0, "distance wraps past the loop length");
    }
    {
        const auto S = FRailPathResolver::Sample(Square, -500.0);
        Check(S.Position.X == 0.0 && S.Position.Y == 500.0, "negative distance wraps backward onto the closing side");
        CheckNear(S.Forward.Y, -1.0, "closing side heads -Y back to the start");
    }
    {
        // Degenerate paths.
        TArray<FVector> One;
        One.Add(FVector(7, 7, 7));
        Check(!FRailPathResolver::Sample(One, 100.0).bValid, "a single point can't define travel -> invalid");
        TArray<FVector> None;
        Check(!FRailPathResolver::Sample(None, 0.0).bValid, "no points -> invalid");
    }

    // --- Train controller envelope. ---
    FTrainMover::FParams P;
    P.TrackLength = 4000.0;
    P.LineSpeed = 2000.0;
    P.Accel = 300.0;
    P.Brake = 400.0;
    P.DwellSeconds = 5.0;
    {
        // No stations: cruise the loop, building toward line speed and wrapping.
        FTrainMover T;
        T.Configure(P, TArray<double>{});
        for (int i = 0; i < 400; ++i)
        {
            T.Advance(0.1);
        }
        Check(T.Speed() > 0.0, "no stations -> the train cruises");
        Check(T.Position() >= 0.0 && T.Position() < P.TrackLength, "position stays within the loop");
    }
    {
        // Two stations: brakes to a dead stop exactly at the platform, then dwells.
        FTrainMover T;
        TArray<double> Stops;
        Stops.Add(1000.0);
        Stops.Add(3000.0);
        T.Configure(P, Stops);
        int guard = 0;
        while (!T.IsDwelling() && guard++ < 5000)
        {
            T.Advance(0.05);
        }
        Check(T.IsDwelling(), "the train reaches the first station and dwells");
        CheckNear(T.Position(), 1000.0, "it stops EXACTLY at the platform");
        Check(T.Speed() == 0.0, "stopped -> zero speed");
        Check(T.TargetStation() == 0, "serving the first station");
    }
    {
        // After the dwell it departs to the NEXT station.
        FTrainMover T;
        TArray<double> Stops;
        Stops.Add(1000.0);
        Stops.Add(3000.0);
        T.Configure(P, Stops);
        int guard = 0;
        while (!T.IsDwelling() && guard++ < 5000) { T.Advance(0.05); } // arrive at station 0
        for (int i = 0; i < 60; ++i) { T.Advance(0.1); }               // wait out the 5s dwell, catch it pulling away
        Check(!T.IsDwelling(), "the dwell ends and the train departs");
        Check(T.TargetStation() == 1, "now heading for the second station");
        Check(T.Speed() > 0.0, "pulling away -> moving again");
    }
    {
        // The braking cap keeps a close station reachable without overshoot.
        FTrainMover T;
        TArray<double> Stops;
        Stops.Add(1000.0);
        Stops.Add(3000.0);
        T.Configure(P, Stops);
        int guard = 0;
        while (T.Position() < 500.0 && guard++ < 5000) { T.Advance(0.02); }
        const double Cap = 900.0; // sqrt(2*400*distToStop), distToStop<=500 -> <= ~632
        Check(T.Speed() <= Cap, "speed stays under the braking-distance cap approaching the stop");
    }

    return OracleSummary("train_oracle");
}
