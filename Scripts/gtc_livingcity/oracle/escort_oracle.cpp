// Out-of-tree oracle for FEscortResolver — compiles + runs the REAL EscortResolver.cpp against the
// REAL EscortObjective.cpp and re-asserts the protect-the-convoy adapter the live
// UGTCEscortObjectiveComponent leans on: escortee route progress + threat-from-hostiles -> the
// integrity push-pull (drain under fire, recover when suppressed, delivered on arrival, lost at 0).
#include "../../../Source/GTC_UE5/Missions/Activities/EscortResolver.h"
#include "../../../Source/GTC_UE5/Missions/Activities/EscortObjective.h"
#include "harness.h"

static FEscortResolver::FOutput Cook(const FVector& Escortee, const FVector& Dest, double Route,
    double Prev, const TArray<FVector>& Hostiles)
{
    FEscortResolver::FParams P; // ArrivalRadius 500, ThreatRadius 3000, ThreatSaturation 3
    FEscortResolver::FInput In;
    In.EscorteePos = Escortee;
    In.Destination = Dest;
    In.RouteLength = Route;
    In.PrevProgress = Prev;
    In.HostilePositions = Hostiles;
    return FEscortResolver::Cook(In, P);
}

int main()
{
    const FVector O(0, 0, 0);
    const TArray<FVector> None;

    // --- Route progress: monotonic high-water toward the destination. ---
    {
        CheckNear(Cook(FVector(1000, 0, 0), O, 1000, 0, None).Progress, 0.0, "at the start -> 0 progress", 1e-6);
        // Halfway of the reachable span (route - arrival radius 500): dist = 500 + 0.5*500 = 750.
        CheckNear(Cook(FVector(750, 0, 0), O, 1000, 0, None).Progress, 0.5, "halfway of the reachable span -> 0.5");
        Check(Cook(FVector(400, 0, 0), O, 1000, 0, None).Progress == 1.0, "within the arrival radius -> delivered progress");
        CheckNear(Cook(FVector(1000, 0, 0), O, 1000, 0.6, None).ProgressDelta, 0.0, "backtracking awards no progress");
    }

    // --- Threat: proximity-weighted hostile count, saturating to 1. ---
    {
        Check(Cook(O, O, 1000, 0, None).ThreatLevel == 0.0, "no hostiles -> no threat");
        CheckNear(Cook(O, O, 1000, 0, TArray<FVector>{ O }).ThreatLevel, 1.0 / 3.0, "one point-blank attacker -> 1/saturation");
        CheckNear(Cook(O, O, 1000, 0, TArray<FVector>{ O, O, O }).ThreatLevel, 1.0, "three point-blank attackers -> full threat");
        Check(Cook(O, O, 1000, 0, TArray<FVector>{ FVector(3000, 0, 0) }).ThreatLevel == 0.0, "an attacker at the threat radius -> no threat");
        CheckNear(Cook(O, O, 1000, 0, TArray<FVector>{ FVector(1500, 0, 0) }).ThreatLevel, 0.5 / 3.0, "an attacker at half range -> half heat");
    }

    // --- End-to-end: escort to the drop with the heat cleared -> delivered, integrity intact. ---
    {
        FEscortObjective E;
        FEscortObjective::FParams EP; // Drain 0.25, Regen 0.1, threshold 0.05
        E.Configure(EP);
        double High = 0.0;
        for (int i = 1; i <= 20; ++i)
        {
            const FVector Pos(FMath::Max(0.0, 10000.0 - i * 600.0), 0, 0); // escortee drives in, no hostiles
            const auto C = Cook(Pos, O, 10000, High, None);
            High = C.Progress;
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        Check(E.IsDelivered(), "a clean escort to the drop delivers");
        CheckNear(E.Integrity(), 1.0, "an unthreatened escortee stays at full integrity");
    }

    // --- End-to-end: sustained heavy fire destroys the escortee. ---
    {
        FEscortObjective E;
        FEscortObjective::FParams EP;
        E.Configure(EP);
        const TArray<FVector> Swarm{ O, O, O }; // three point-blank -> full threat
        for (int i = 0; i < 6; ++i)
        {
            const auto C = Cook(O, FVector(10000, 0, 0), 10000, 0, Swarm); // pinned, not progressing
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        Check(E.IsLost(), "sustained full threat drains integrity to zero (escortee lost)");
    }

    // --- End-to-end: suppress the attackers and a battered escortee recovers. ---
    {
        FEscortObjective E;
        FEscortObjective::FParams EP;
        E.Configure(EP);
        const TArray<FVector> Swarm{ O, O, O };
        for (int i = 0; i < 2; ++i)
        {
            const auto C = Cook(O, FVector(10000, 0, 0), 10000, 0, Swarm); // 2s of full threat
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        const double Hurt = E.Integrity();
        Check(Hurt < 1.0, "taking fire drops integrity");
        for (int i = 0; i < 3; ++i)
        {
            const auto C = Cook(O, FVector(10000, 0, 0), 10000, 0, None); // attackers cleared
            E.Update(C.ProgressDelta, C.ThreatLevel, 1.0);
        }
        Check(E.Integrity() > Hurt, "with the threat suppressed the escortee recovers");
        Check(E.IsEscorting(), "recovering, the escort is still live");
    }

    return OracleSummary("escort_oracle");
}
