// Headless verifier for FPoliceSpawnPlan. Compiles the REAL shipping cores
// (PoliceResponse / PoliceDispatch / PoliceEscalation / PoliceSpawnPlan) against
// the host-clang shim and re-asserts the same structural invariants the
// GTC.AI.PoliceDispatch.PoliceSpawnPlan automation test checks — so the actual
// game math is exercised with a plain compiler, the hook-safe way (the in-editor
// UE automation runner is intentionally NOT launched).
//
// NOTE: the shim's FRandomStream is a stand-in LCG, so absolute ring positions
// differ from an in-engine run; only seed-determinism and the geometry/mix
// invariants (which the real test also asserts) are checked here.
#include <cstdio>
#include <cmath>

#include "../../Source/GTC_UE5/World/Police/PoliceResponse.cpp"
#include "../../Source/GTC_UE5/AI/PoliceDispatch/PoliceDispatch.cpp"
#include "../../Source/GTC_UE5/AI/PoliceDispatch/PoliceEscalation.cpp"
#include "../../Source/GTC_UE5/AI/PoliceDispatch/PoliceSpawnPlan.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)

static double PlanarLen(const FVector& V) { return std::sqrt(V.X * V.X + V.Z * V.Z); }

int main()
{
    const double Eps = 1e-4;

    // 1. Empty when not wanted, or when the field is already satisfied.
    {
        FRandomStream Rng(1234);
        CHECK(FPoliceSpawnPlan::Build(0, 0, 12, 8, 50.0, Rng).Num() == 0, "zero stars -> empty plan");
        CHECK(FPoliceSpawnPlan::Build(2, 2, 12, 8, 50.0, Rng).Num() == 0, "satisfied wave -> empty");
    }

    // 2. Wave size matches the dispatch deficit (and respects the per-wave cap).
    {
        FRandomStream Rng(99);
        const int32 Expected = FPoliceDispatch::SpawnCount(5, 0, 12, 99);
        CHECK(FPoliceSpawnPlan::Build(5, 0, 12, 99, 50.0, Rng).Num() == Expected, "wave fills the deficit");
        CHECK(FPoliceSpawnPlan::Build(5, 0, 12, 3, 50.0, Rng).Num() == 3, "wave respects per-wave cap");
    }

    // 3. Ring geometry (planar, jittered radius) + unit mix membership.
    {
        FRandomStream Rng(7);
        const double Radius = 50.0;
        const double Jitter = FPoliceSpawnPlan::DefaultRadialJitterMeters;
        const TArray<EPoliceUnitType> Mix = FPoliceEscalation::ResponseUnits(5);
        const TArray<FPoliceSpawnSlot> Plan = FPoliceSpawnPlan::Build(5, 0, 12, 99, Radius, Rng);
        CHECK(Plan.Num() > 0, "plan non-empty at 5 stars");
        for (const FPoliceSpawnSlot& Slot : Plan)
        {
            CHECK(std::fabs(Slot.PlanarOffset.Y) < Eps, "offset is planar (Y==0)");
            const double Len = PlanarLen(Slot.PlanarOffset);
            CHECK(Len >= Radius - Jitter - Eps && Len <= Radius + Jitter + Eps, "offset within jittered ring");
            CHECK(Mix.Contains(Slot.UnitType), "unit type is in the escalation mix");
        }
    }

    // 4. Determinism (same seed -> identical plan) + round-robin unit dealing.
    {
        FRandomStream A(42), B(42);
        const TArray<FPoliceSpawnSlot> PlanA = FPoliceSpawnPlan::Build(4, 1, 12, 99, 60.0, A);
        const TArray<FPoliceSpawnSlot> PlanB = FPoliceSpawnPlan::Build(4, 1, 12, 99, 60.0, B);
        CHECK(PlanA.Num() == PlanB.Num(), "same seed -> same wave size");
        bool bSame = PlanA.Num() == PlanB.Num();
        for (int32 i = 0; bSame && i < PlanA.Num(); ++i)
        {
            bSame = PlanA[i].UnitType == PlanB[i].UnitType
                && std::fabs(PlanA[i].PlanarOffset.X - PlanB[i].PlanarOffset.X) < Eps
                && std::fabs(PlanA[i].PlanarOffset.Z - PlanB[i].PlanarOffset.Z) < Eps;
        }
        CHECK(bSame, "same seed -> identical plan");

        const TArray<EPoliceUnitType> Mix = FPoliceEscalation::ResponseUnits(3);
        FRandomStream C(1);
        const TArray<FPoliceSpawnSlot> Plan3 = FPoliceSpawnPlan::Build(3, 0, 12, 4, 50.0, C);
        CHECK(Plan3.Num() == Mix.Num(), "wave equals mix size when cap == desired");
        bool bOrdered = Plan3.Num() == Mix.Num();
        for (int32 i = 0; bOrdered && i < Plan3.Num(); ++i)
        {
            bOrdered = Plan3[i].UnitType == Mix[i];
        }
        CHECK(bOrdered, "units dealt in mix order");
    }

    if (g_fail != 0)
    {
        std::printf("=== police_spawn_plan_verify FAILED (%d) ===\n", g_fail);
        return 1;
    }
    std::printf("=== police_spawn_plan_verify PASSED ===\n");
    return 0;
}
