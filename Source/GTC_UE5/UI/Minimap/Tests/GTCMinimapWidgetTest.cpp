// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCMinimapWidget.h"
#include "../../../Tests/GtcTestTolerances.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

using GtcTest::Eps;

// ============================================================================
// GTC.UI.Minimap — behavior test for the radar C++ base. A transient
// UGTCMinimapWidget (NewObject, no Slate) is fed a center/heading, a marker set,
// and an L-shaped route, then Refresh() must project the markers (culling
// off-range, pinning sticky ones to the rim) and derive the GPS readout
// (distance / ETA / next turn) from the route. The visual WBP is DEFERRED.
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGTCMinimapWidgetRefreshTest,
    "GTC.UI.Minimap.WidgetRefresh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGTCMinimapWidgetRefreshTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("transient world created"), World))
    {
        return false;
    }
    FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
    Ctx.SetCurrentWorld(World);

    AActor* Owner = World->SpawnActor<AActor>();
    UGTCMinimapWidget* Map = NewObject<UGTCMinimapWidget>(Owner);
    if (!TestNotNull(TEXT("minimap widget created"), Map))
    {
        return false;
    }

    Map->RangeCm = 10000.0f;             // 100 m radius
    Map->SetCenterAndHeading(FVector::ZeroVector, 0.0f); // facing +X
    Map->SetPlayerSpeed(1000.0f);        // 10 m/s

    // --- Markers: one in-range, one sticky-far (edge), one plain-far (culled) ----
    TArray<FGTCMinimapMarker> Markers;
    { FGTCMinimapMarker M; M.WorldLocation = FVector(5000, 0, 0); M.Kind = "diner";     M.bClampToEdge = false; Markers.Add(M); }
    { FGTCMinimapMarker M; M.WorldLocation = FVector(50000, 0, 0); M.Kind = "objective"; M.bClampToEdge = true;  Markers.Add(M); }
    { FGTCMinimapMarker M; M.WorldLocation = FVector(0, 50000, 0); M.Kind = "npc";       M.bClampToEdge = false; Markers.Add(M); }
    Map->SetMarkers(Markers);

    // --- Route: 10 m east, then 10 m north (a right turn at the bend) -------------
    Map->SetRoute({ FVector(0, 0, 0), FVector(1000, 0, 0), FVector(1000, 1000, 0) });

    Map->Refresh();

    // Plain-far marker culled; in-range + sticky-far survive.
    TestEqual(TEXT("two markers drawn"), Map->ProjectedMarkers.Num(), 2);

    bool bSawDinerInside = false;
    bool bSawObjectiveOnEdge = false;
    for (const FGTCMinimapProjectedMarker& P : Map->ProjectedMarkers)
    {
        if (P.Kind == FName("diner"))
        {
            bSawDinerInside = !P.bOnEdge && FMath::Abs(P.Normalized.Y - 0.5f) < Eps;
        }
        if (P.Kind == FName("objective"))
        {
            bSawObjectiveOnEdge = P.bOnEdge && FMath::Abs(P.Normalized.Size() - 1.0f) < Eps;
        }
    }
    TestTrue(TEXT("diner projected inside the disc"), bSawDinerInside);
    TestTrue(TEXT("objective pinned to the rim"), bSawObjectiveOnEdge);

    // GPS readout: 20 m remaining, 2 s ETA, a right turn 10 m ahead.
    TestTrue(TEXT("has route"), Map->bHasRoute);
    TestEqual(TEXT("distance ~20 m"), Map->RouteDistanceM, 20.0f, 0.01f);
    TestEqual(TEXT("eta 0:02"), Map->RouteEtaText, FString(TEXT("0:02")));
    TestEqual(TEXT("next turn right 10 m"), Map->NextTurnText, FString(TEXT("Right 10 m")));
    TestEqual(TEXT("route has 3 points"), Map->ProjectedRoute.Num(), 3);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
