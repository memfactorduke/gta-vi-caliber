// Headless verifier for FFastTravelNetwork. Compiles the REAL core .cpp against the shim
// and re-asserts the GTC.World.FastTravel invariants with a host clang.
#include <cstdio>

#include "../../Source/GTC_UE5/World/FastTravel/FastTravelNetwork.cpp"

using FHub = FFastTravelNetwork::FHub;

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static double Absd(double v) { return v < 0 ? -v : v; }

int main()
{
    // Hubs along +X: A marina @0, B helipad @1km, C airstrip @3km, D marina @0.5km LOCKED.
    TArray<FHub> Hubs = {
        FHub{ FVector(0.0, 0.0, 0.0),       EHubKind::Marina,   1, true  },
        FHub{ FVector(100000.0, 0.0, 0.0),  EHubKind::Helipad,  2, true  },
        FHub{ FVector(300000.0, 0.0, 0.0),  EHubKind::Airstrip, 3, true  },
        FHub{ FVector(50000.0, 0.0, 0.0),   EHubKind::Marina,   4, false },
    };

    // ---- NearestHub ------------------------------------------------------------
    CHECK(FFastTravelNetwork::NearestHub(Hubs, FVector(0, 0, 0), 0.0) == 0, "at A -> A (unlimited)");
    CHECK(FFastTravelNetwork::NearestHub(Hubs, FVector(60000, 0, 0), 0.0) == 1, "near B -> B (D is locked)");
    CHECK(FFastTravelNetwork::NearestHub(Hubs, FVector(0, 0, 0), 30000.0) == 0, "A within range");
    CHECK(FFastTravelNetwork::NearestHub(Hubs, FVector(200000, 0, 0), 30000.0) == FFastTravelNetwork::None,
        "nothing within 0.3km -> None");

    // ---- NearestHubOfKind ------------------------------------------------------
    CHECK(FFastTravelNetwork::NearestHubOfKind(Hubs, FVector(60000, 0, 0), EHubKind::Marina, 0.0) == 0,
        "nearest unlocked marina is A (D locked, closer)");
    CHECK(FFastTravelNetwork::NearestHubOfKind(Hubs, FVector(0, 0, 0), EHubKind::Airstrip, 0.0) == 2,
        "only airstrip is C");

    // ---- Ties resolve to the lower index ---------------------------------------
    {
        TArray<FHub> Pair = {
            FHub{ FVector(100.0, 0.0, 0.0),  EHubKind::Helipad, 1, true },
            FHub{ FVector(-100.0, 0.0, 0.0), EHubKind::Helipad, 2, true },
        };
        CHECK(FFastTravelNetwork::NearestHub(Pair, FVector(0, 0, 0), 0.0) == 0, "equidistant -> lower index");
    }

    // ---- Fare ------------------------------------------------------------------
    CHECK(FFastTravelNetwork::Fare(Hubs[0], Hubs[1], 5.0, 10) == 15, "1km @ 5/km + 10 base = 15");
    CHECK(FFastTravelNetwork::Fare(Hubs[0], Hubs[2], 5.0, 10) == 25, "3km @ 5/km + 10 base = 25");
    CHECK(FFastTravelNetwork::Fare(Hubs[0], Hubs[0], 5.0, 10) == 10, "same hub -> base fare");
    CHECK(FFastTravelNetwork::Fare(Hubs[0], Hubs[1], -5.0, 10) == 10, "negative rate clamps to base");

    // ---- TravelSeconds ---------------------------------------------------------
    CHECK(Absd(FFastTravelNetwork::TravelSeconds(Hubs[0], Hubs[1], 5000.0) - 20.0) < 1e-9, "1km / 5000cms = 20s");
    CHECK(FFastTravelNetwork::TravelSeconds(Hubs[0], Hubs[1], 0.0) > 0.0, "zero speed doesn't divide by zero");

    // ---- CanDepart -------------------------------------------------------------
    CHECK(!FFastTravelNetwork::CanDepart(true, 9999.0, 200.0), "wanted -> cannot depart");
    CHECK(FFastTravelNetwork::CanDepart(false, 500.0, 200.0), "clear + no near threat -> can depart");
    CHECK(!FFastTravelNetwork::CanDepart(false, 100.0, 200.0), "threat too close -> cannot depart");

    if (g_fail == 0) { std::printf("fast_travel_verify: ALL PASS\n"); return 0; }
    std::printf("fast_travel_verify: %d FAILED\n", g_fail);
    return 1;
}
