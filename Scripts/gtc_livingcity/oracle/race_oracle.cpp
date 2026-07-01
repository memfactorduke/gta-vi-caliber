// Out-of-tree oracle for FRaceProgressResolver + the race cores — compiles + runs the REAL
// RaceProgressResolver.cpp / RaceSession.cpp / StreetRace.cpp / RivalPacer.cpp and drives a full
// street race the way the live UGTCStreetRaceComponent does: map UE positions into the cores'
// XZ ground plane, pace the rivals down the course, cross the player's gates from its position.
#include "../../../Source/GTC_UE5/Missions/Activities/RaceProgressResolver.h"
#include "../../../Source/GTC_UE5/Missions/Activities/RaceSession.h"
#include "harness.h"

// Replicate the component's per-tick rival driving: advance each rival's gates until its cleared
// count catches where its pace clock says it should be.
static void DriveRivals(FRaceSession& S, TArray<double>& RivalProg, const TArray<double>& Paces,
    int32 Total, double Radius)
{
    const double PlayerProg = S.ProgressOf(0);
    for (int32 r = 0; r < Paces.Num(); ++r)
    {
        const int32 Idx = r + 1;
        const auto RT = FRaceProgressResolver::RivalTick(
            RivalProg[r], S.Participant(Idx).Elapsed(), Paces[r], PlayerProg, 0.0, 0.0, Total);
        RivalProg[r] = RT.Progress;
        int32 Guard = 0;
        while (Guard++ <= Total && !S.Participant(Idx).IsFinished()
            && (Total - S.Participant(Idx).CheckpointsRemaining()) < RT.TargetCleared)
        {
            if (!S.Reached(Idx, S.Participant(Idx).CurrentCheckpoint(), Radius))
            {
                break;
            }
        }
    }
}

int main()
{
    // --- Coordinate bridge: UE.X -> X, UE.Y -> Z, height dropped. ---
    {
        const FVector M = FRaceProgressResolver::ToRaceSpace(FVector(10, 20, 30));
        Check(M.X == 10.0 && M.Y == 0.0 && M.Z == 20.0, "ToRaceSpace routes UE X->X, UE Y->Z, drops height");
    }

    // --- Rival pacing: a rival at half its pace time is halfway (2 of 4 gates); monotonic. ---
    {
        const auto RT = FRaceProgressResolver::RivalTick(0.0, 10.0, 20.0, 0.0, 0.0, 0.0, 4);
        CheckNear(RT.Progress, 0.5, "rival at half its pace time is halfway");
        Check(RT.TargetCleared == 2, "half progress on a 4-gate course = 2 gates");
        const auto RT2 = FRaceProgressResolver::RivalTick(0.8, 10.0, 20.0, 0.0, 0.0, 0.0, 4);
        CheckNear(RT2.Progress, 0.8, "progress never drops below the previous high-water");
    }

    // --- Full race: player + 2 rivals, player finishes fast -> wins; rivals ranked by pace. ---
    {
        const TArray<FVector> Ue = { FVector(0, 0, 0), FVector(1000, 0, 0), FVector(1000, 1000, 0), FVector(0, 1000, 0) };
        TArray<FVector> Race;
        for (const auto& C : Ue)
        {
            Race.Add(FRaceProgressResolver::ToRaceSpace(C));
        }
        const double Radius = 800.0;
        const int32 Total = 4; // 4 gates * 1 lap
        FRaceSession S(3, Race, 1, 3.0, 1000);
        TArray<double> Paces = { 20.0, 40.0 };
        TArray<double> RivalProg = { 0.0, 0.0 };

        // Countdown refuses gate crossings; three 1s ticks bring the lights green.
        Check(!S.Reached(0, Race[0], Radius), "no gates cross during the countdown");
        S.Tick(1.0);
        S.Tick(1.0);
        S.Tick(1.0);
        Check(S.IsRacing(), "race goes green after the countdown");

        // The player drives through all four gates on consecutive racing ticks.
        for (int32 g = 0; g < 4; ++g)
        {
            S.Tick(1.0);
            DriveRivals(S, RivalProg, Paces, Total, Radius);
            S.Reached(0, FRaceProgressResolver::ToRaceSpace(Ue[g]), Radius);
        }
        Check(S.Participant(0).IsFinished(), "the fast player finishes all four gates");
        Check(S.PlacementOf(0) == 1, "having finished first, the player leads");

        // Keep ticking until the field settles on the rivals' pace clocks.
        int32 Safety = 0;
        while (!S.IsFinished() && Safety++ < 500)
        {
            S.Tick(1.0);
            DriveRivals(S, RivalProg, Paces, Total, Radius);
        }
        Check(S.IsFinished(), "the race finishes once every rival is in");
        Check(S.PlacementOf(0) == 1, "the fast player wins (placement 1)");
        Check(S.PlacementOf(1) == 2, "the faster rival (pace 20) places 2nd");
        Check(S.PlacementOf(2) == 3, "the slower rival (pace 40) places 3rd");

        const auto Results = S.Results();
        Check(Results.Num() == 3, "three ranked results");
        Check(Results[0].ParticipantIndex == 0 && Results[0].Placement == 1, "player tops the tally");
        Check(Results[0].Reward == StreetRace::Reward(1, 1000), "1st pays the full-scaled reward");
        Check(Results[0].bFinished, "the player is a finisher");
    }

    // --- The mapping matters: a gate hit tracks UE horizontal distance, not height. ---
    {
        const TArray<FVector> Ue = { FVector(0, 0, 0), FVector(5000, 0, 0) };
        TArray<FVector> Race;
        for (const auto& C : Ue)
        {
            Race.Add(FRaceProgressResolver::ToRaceSpace(C));
        }
        FRaceSession S(1, Race, 1, 0.0, 0); // solo, zero countdown -> racing at once
        Check(S.IsRacing(), "a solo zero-countdown race starts green");
        Check(!S.Reached(0, FRaceProgressResolver::ToRaceSpace(FVector(0, 5000, 0)), 800.0),
            "a gate 5000 away on UE-Y is not counted (horizontal distance preserved)");
        Check(S.Reached(0, FRaceProgressResolver::ToRaceSpace(FVector(0, 0, 9999)), 800.0),
            "the same gate IS counted from directly above (UE height ignored)");
    }

    return OracleSummary("race_oracle");
}
