// Out-of-tree oracle for FCrowdBudget — compiles + runs the REAL CrowdBudget.cpp
// and re-asserts GTC.NPC.CrowdBudget.
#include "../../../Source/GTC_UE5/NPC/Population/CrowdBudget.h"
#include "harness.h"

int main()
{
    Check(FCrowdBudget::SpawnCount(38, 40, 4) == 2, "small deficit spawns the deficit");
    Check(FCrowdBudget::SpawnCount(40, 40, 4) == 0, "at target spawns nothing");
    Check(FCrowdBudget::SpawnCount(10, 40, 4) == 4, "big deficit is capped per pass");
    Check(FCrowdBudget::SpawnCount(45, 40, 4) == 0, "over target spawns nothing");

    Check(FCrowdBudget::ShouldDespawn(9000, 8000), "beyond radius despawns");
    Check(!FCrowdBudget::ShouldDespawn(5000, 8000), "inside radius stays");
    Check(!FCrowdBudget::ShouldDespawn(8000, 8000), "exactly at radius stays");

    Check(FCrowdBudget::InSpawnRing(4000, 2500, 6000), "in the ring");
    Check(FCrowdBudget::InSpawnRing(2500, 2500, 6000), "on the inner edge");
    Check(!FCrowdBudget::InSpawnRing(1000, 2500, 6000), "too close");
    Check(!FCrowdBudget::InSpawnRing(7000, 2500, 6000), "too far");

    CheckNear(FCrowdBudget::TickInterval(3000, 4000, 0.2), 0.0, "near ticks every frame");
    CheckNear(FCrowdBudget::TickInterval(5000, 4000, 0.2), 0.2, "far ticks on the coarse interval");

    {
        const TArray<double> D({1000.0, 9000.0, 8500.0, 100.0});
        Check(FCrowdBudget::FarthestBeyond(D, 8000.0) == 1, "retires the farthest over-radius citizen");
        Check(FCrowdBudget::FarthestBeyond(TArray<double>({100.0, 200.0}), 8000.0) == -1, "none beyond -> -1");
        Check(FCrowdBudget::FarthestBeyond(TArray<double>({9000.0, 9000.0, 100.0}), 8000.0) == 0, "tie keeps the lower index");
    }

    return OracleSummary("FCrowdBudget");
}
