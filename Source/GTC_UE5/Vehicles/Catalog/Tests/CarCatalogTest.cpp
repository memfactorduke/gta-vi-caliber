// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CarCatalog.h"
#include "Math/RandomStream.h"

// Pure-core coverage for the car roster (FGTCCarCatalog). No engine/world needed.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarCatalogDefaultRosterTest,
    "GTC.Vehicles.Catalog.DefaultRosterLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarCatalogDefaultRosterTest::RunTest(const FString& Parameters)
{
    FGTCCarCatalog Cat;
    TestEqual(TEXT("13 default cars"), Cat.Num(), 13);
    TestTrue(TEXT("has riptide"), Cat.Has(TEXT("riptide")));
    TestTrue(TEXT("has coral-ss"), Cat.Has(TEXT("coral-ss")));
    TestFalse(TEXT("no unknown"), Cat.Has(TEXT("nope")));
    // The towed trailer is intentionally not a standalone car.
    TestFalse(TEXT("trailer excluded"), Cat.Has(TEXT("trailer")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarCatalogAssetBindingsTest,
    "GTC.Vehicles.Catalog.AssetBindings",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarCatalogAssetBindingsTest::RunTest(const FString& Parameters)
{
    FGTCCarCatalog Cat;
    const FCarModel* Riptide = Cat.Find(TEXT("riptide"));
    TestNotNull(TEXT("riptide found"), Riptide);
    if (Riptide)
    {
        TestEqual(TEXT("riptide pawn bp"), Riptide->PawnBlueprintPath,
            FString(TEXT("/Game/CitySampleVehicles/vehicle12_Car/BP_vehicle12_Car.BP_vehicle12_Car_C")));
        // Riptide (vehicle12) is a recolor variant that reuses vehicle03's body mesh.
        TestEqual(TEXT("riptide mesh = vehicle03 body"), Riptide->BodyMeshPath,
            FString(TEXT("/Game/CitySampleVehicles/vehicle03_Car/Mesh/SKM_vehCar_vehicle03")));
        // Every binding must be a _C class path and live under the pack.
        TestTrue(TEXT("bp is a class path"), Riptide->PawnBlueprintPath.EndsWith(TEXT("_C")));
    }
    for (const FString& Id : Cat.Ids())
    {
        const FCarModel* Car = Cat.Find(Id);
        TestTrue(TEXT("under pack"), Car && Car->PawnBlueprintPath.StartsWith(TEXT("/Game/CitySampleVehicles/")));
        TestTrue(TEXT("mesh under pack"), Car && Car->BodyMeshPath.StartsWith(TEXT("/Game/CitySampleVehicles/")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarCatalogMalformedDroppedTest,
    "GTC.Vehicles.Catalog.MalformedDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarCatalogMalformedDroppedTest::RunTest(const FString& Parameters)
{
    TArray<FCarModel> Defs;
    FCarModel Ok;       Ok.Id = TEXT("ok");       Ok.PawnBlueprintPath = TEXT("/Game/X.X_C");
    FCarModel NoId;     NoId.Id = TEXT("");        NoId.PawnBlueprintPath = TEXT("/Game/X.X_C");
    FCarModel NoBp;     NoBp.Id = TEXT("nobp");    NoBp.PawnBlueprintPath = TEXT("");
    FCarModel Dup;      Dup.Id = TEXT("ok");       Dup.PawnBlueprintPath = TEXT("/Game/Y.Y_C");
    Defs.Add(Ok);
    Defs.Add(NoId);
    Defs.Add(NoBp);
    Defs.Add(Dup);
    FGTCCarCatalog Cat(Defs);
    TestEqual(TEXT("only the one valid car"), Cat.Num(), 1);
    TestTrue(TEXT("has ok"), Cat.Has(TEXT("ok")));
    const FCarModel* C = Cat.Find(TEXT("ok"));
    TestTrue(TEXT("first wins on dup"), C && C->PawnBlueprintPath == TEXT("/Game/X.X_C"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarCatalogClassFilterTest,
    "GTC.Vehicles.Catalog.ClassFilterAndIds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarCatalogClassFilterTest::RunTest(const FString& Parameters)
{
    FGTCCarCatalog Cat;
    // Sedans: Surfline + Esplanade.
    const TArray<FCarModel> Sedans = Cat.OfClass(EGTCVehicleClass::Sedan);
    TestEqual(TEXT("two sedans"), Sedans.Num(), 2);
    // Class ids align with FChopShop's taxonomy.
    TestEqual(TEXT("sports id"), FGTCCarCatalog::ClassId(EGTCVehicleClass::Sports), FString(TEXT("sports")));
    TestEqual(TEXT("super id"), FGTCCarCatalog::ClassId(EGTCVehicleClass::Super), FString(TEXT("super")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarCatalogStatSanityTest,
    "GTC.Vehicles.Catalog.StatSanity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarCatalogStatSanityTest::RunTest(const FString& Parameters)
{
    FGTCCarCatalog Cat;
    const FCarModel* Coral = Cat.Find(TEXT("coral-ss"));   // super
    const FCarModel* Tide = Cat.Find(TEXT("tidewater"));   // compact
    TestNotNull(TEXT("coral"), Coral);
    TestNotNull(TEXT("tide"), Tide);
    if (Coral && Tide)
    {
        TestTrue(TEXT("super faster than compact"), Coral->TopSpeedKmh > Tide->TopSpeedKmh);
        TestTrue(TEXT("super worth more"), Coral->ChopValue > Tide->ChopValue);
        // cm/s conversion: 330 km/h ~= 9166.7 cm/s.
        TestTrue(TEXT("cms conversion"), FMath::IsNearlyEqual(Coral->BaseTopSpeedCms(), 330.0 * 1000.0 / 36.0, 0.5));
    }
    // Every car has positive footprint, tank, value, and a seat.
    for (const FString& Id : Cat.Ids())
    {
        const FCarModel* Car = Cat.Find(Id);
        if (!Car) { continue; }
        TestTrue(TEXT("positive top speed"), Car->TopSpeedKmh > 0.0);
        TestTrue(TEXT("positive tank"), Car->TankLitres > 0.0);
        TestTrue(TEXT("positive chop value"), Car->ChopValue > 0);
        TestTrue(TEXT("has seats"), Car->Seats > 0);
        TestTrue(TEXT("nonzero footprint"), Car->BodyHalfExtent.X > 0.0);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCarCatalogRandomDeterministicTest,
    "GTC.Vehicles.Catalog.RandomDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCarCatalogRandomDeterministicTest::RunTest(const FString& Parameters)
{
    FGTCCarCatalog Cat;
    FRandomStream A(1234);
    FRandomStream B(1234);
    const FCarModel* PickA = Cat.Random(A);
    const FCarModel* PickB = Cat.Random(B);
    TestNotNull(TEXT("pick a"), PickA);
    TestNotNull(TEXT("pick b"), PickB);
    TestTrue(TEXT("same seed same car"), PickA && PickB && PickA->Id == PickB->Id);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
