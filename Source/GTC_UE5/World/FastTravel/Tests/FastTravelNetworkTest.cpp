// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../FastTravelNetwork.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;
using FHub = FFastTravelNetwork::FHub;

/**
 * Tests for FFastTravelNetwork — the hub graph behind map fast travel. GTC-original.
 * Pins nearest-hub (overall / of kind / range-limited / locked-skipped / ties), fare,
 * travel time, and the no-fast-travel-while-hot departure rule. Mirrors
 * Scripts/gtc_fasttravel/fast_travel_verify.cpp. Prefix GTC.World.FastTravel.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FFastTravelNetworkTest,
    "GTC.World.FastTravel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FFastTravelNetworkTest::RunTest(const FString& Parameters)
{
    TArray<FHub> Hubs = {
        FHub{ FVector(0.0, 0.0, 0.0),      EHubKind::Marina,   1, true  },
        FHub{ FVector(100000.0, 0.0, 0.0), EHubKind::Helipad,  2, true  },
        FHub{ FVector(300000.0, 0.0, 0.0), EHubKind::Airstrip, 3, true  },
        FHub{ FVector(50000.0, 0.0, 0.0),  EHubKind::Marina,   4, false },
    };

    // ---- NearestHub -----------------------------------------------------------
    TestEqual(TEXT("at A -> A"), FFastTravelNetwork::NearestHub(Hubs, FVector(0, 0, 0), 0.0), 0);
    TestEqual(TEXT("near B -> B (D locked)"), FFastTravelNetwork::NearestHub(Hubs, FVector(60000, 0, 0), 0.0), 1);
    TestEqual(TEXT("A within range"), FFastTravelNetwork::NearestHub(Hubs, FVector(0, 0, 0), 30000.0), 0);
    TestEqual(TEXT("nothing within 0.3km -> None"),
        FFastTravelNetwork::NearestHub(Hubs, FVector(200000, 0, 0), 30000.0), FFastTravelNetwork::None);

    // ---- NearestHubOfKind -----------------------------------------------------
    TestEqual(TEXT("nearest unlocked marina is A"),
        FFastTravelNetwork::NearestHubOfKind(Hubs, FVector(60000, 0, 0), EHubKind::Marina, 0.0), 0);
    TestEqual(TEXT("only airstrip is C"),
        FFastTravelNetwork::NearestHubOfKind(Hubs, FVector(0, 0, 0), EHubKind::Airstrip, 0.0), 2);

    // ---- Ties -> lower index --------------------------------------------------
    {
        TArray<FHub> Pair = {
            FHub{ FVector(100.0, 0.0, 0.0),  EHubKind::Helipad, 1, true },
            FHub{ FVector(-100.0, 0.0, 0.0), EHubKind::Helipad, 2, true },
        };
        TestEqual(TEXT("equidistant -> lower index"), FFastTravelNetwork::NearestHub(Pair, FVector(0, 0, 0), 0.0), 0);
    }

    // ---- Fare -----------------------------------------------------------------
    TestEqual(TEXT("1km @ 5/km + 10"), FFastTravelNetwork::Fare(Hubs[0], Hubs[1], 5.0, 10), 15);
    TestEqual(TEXT("3km @ 5/km + 10"), FFastTravelNetwork::Fare(Hubs[0], Hubs[2], 5.0, 10), 25);
    TestEqual(TEXT("same hub -> base"), FFastTravelNetwork::Fare(Hubs[0], Hubs[0], 5.0, 10), 10);
    TestEqual(TEXT("negative rate -> base"), FFastTravelNetwork::Fare(Hubs[0], Hubs[1], -5.0, 10), 10);

    // ---- TravelSeconds --------------------------------------------------------
    TestEqual(TEXT("1km / 5000cms = 20s"), FFastTravelNetwork::TravelSeconds(Hubs[0], Hubs[1], 5000.0), 20.0, Eps);
    TestTrue(TEXT("zero speed doesn't divide by zero"),
        FFastTravelNetwork::TravelSeconds(Hubs[0], Hubs[1], 0.0) > 0.0);

    // ---- CanDepart ------------------------------------------------------------
    TestFalse(TEXT("wanted -> cannot depart"), FFastTravelNetwork::CanDepart(true, 9999.0, 200.0));
    TestTrue(TEXT("clear -> can depart"), FFastTravelNetwork::CanDepart(false, 500.0, 200.0));
    TestFalse(TEXT("threat too close -> cannot depart"), FFastTravelNetwork::CanDepart(false, 100.0, 200.0));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
