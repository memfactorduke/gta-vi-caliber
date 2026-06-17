// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GarageStorage.h"

// Each test below maps 1:1 to an assertion in the the reference reference behavior
// game/tests/unit/test_garage_storage.gd (21 tests).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageStoreOccupiesTest,
    "GTC.Vehicles.Ownership.GarageStorage.StoreAddsAndOccupiesSpace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageStoreOccupiesTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    const bool bOk = G.Store(TEXT("home"), TEXT("car1"));
    TestTrue(TEXT("store ok"), bOk);
    TestTrue(TEXT("is_stored"), G.IsStored(TEXT("car1")));
    TestEqual(TEXT("garage == home"), G.GarageOf(TEXT("car1")), FString(TEXT("home")));
    TestEqual(TEXT("count == 1"), G.CountIn(TEXT("home")), 1);
    TestEqual(TEXT("free == 3"), G.FreeSpace(TEXT("home")), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageFullRejectsTest,
    "GTC.Vehicles.Ownership.GarageStorage.FullGarageRejects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageFullRejectsTest::RunTest(const FString& Parameters)
{
    GarageStorage G(2);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Store(TEXT("home"), TEXT("car2"));
    const bool bOk = G.Store(TEXT("home"), TEXT("car3"));
    TestFalse(TEXT("third rejected"), bOk);
    TestEqual(TEXT("count == 2"), G.CountIn(TEXT("home")), 2);
    TestFalse(TEXT("car3 not stored"), G.IsStored(TEXT("car3")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageDoubleStoreTest,
    "GTC.Vehicles.Ownership.GarageStorage.DoubleStoreRejects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageDoubleStoreTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    const bool bAgain = G.Store(TEXT("home"), TEXT("car1"));
    const bool bElsewhere = G.Store(TEXT("garage2"), TEXT("car1"));
    TestFalse(TEXT("again rejected"), bAgain);
    TestFalse(TEXT("elsewhere rejected"), bElsewhere);
    TestEqual(TEXT("total == 1"), G.TotalStored(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRetrieveRemovesTest,
    "GTC.Vehicles.Ownership.GarageStorage.RetrieveRemoves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRetrieveRemovesTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    const bool bOk = G.Retrieve(TEXT("home"), TEXT("car1"));
    TestTrue(TEXT("retrieve ok"), bOk);
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    TestEqual(TEXT("count == 0"), G.CountIn(TEXT("home")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRetrieveWrongGarageTest,
    "GTC.Vehicles.Ownership.GarageStorage.RetrieveWrongGarageFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRetrieveWrongGarageTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    const bool bOk = G.Retrieve(TEXT("garage2"), TEXT("car1"));
    TestFalse(TEXT("wrong garage fails"), bOk);
    TestTrue(TEXT("still stored"), G.IsStored(TEXT("car1")));
    TestEqual(TEXT("garage == home"), G.GarageOf(TEXT("car1")), FString(TEXT("home")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRetrieveMissingTest,
    "GTC.Vehicles.Ownership.GarageStorage.RetrieveMissingVehicleFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRetrieveMissingTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    const bool bOk = G.Retrieve(TEXT("home"), TEXT("ghost"));
    TestFalse(TEXT("missing fails"), bOk);
    TestEqual(TEXT("count == 1"), G.CountIn(TEXT("home")), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageGarageOfOutTest,
    "GTC.Vehicles.Ownership.GarageStorage.GarageOfOutVehicleIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageGarageOfOutTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    TestEqual(TEXT("garage == empty"), G.GarageOf(TEXT("car1")), FString());
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageContentsListsTest,
    "GTC.Vehicles.Ownership.GarageStorage.ContentsListsStored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageContentsListsTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Store(TEXT("home"), TEXT("car2"));
    const TArray<FString> C = G.Contents(TEXT("home"));
    TestEqual(TEXT("size == 2"), C.Num(), 2);
    TestTrue(TEXT("has car1"), C.Contains(TEXT("car1")));
    TestTrue(TEXT("has car2"), C.Contains(TEXT("car2")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageContentsCopyTest,
    "GTC.Vehicles.Ownership.GarageStorage.ContentsIsACopy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageContentsCopyTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    TArray<FString> C = G.Contents(TEXT("home"));
    C.Add(TEXT("hacked"));
    // Mutating the returned copy must not touch the garage.
    TestEqual(TEXT("count == 1"), G.CountIn(TEXT("home")), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageFreeSpaceUnknownTest,
    "GTC.Vehicles.Ownership.GarageStorage.FreeSpaceUnknownGarageIsCapacity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageFreeSpaceUnknownTest::RunTest(const FString& Parameters)
{
    GarageStorage G(3);
    TestEqual(TEXT("free == 3"), G.FreeSpace(TEXT("nowhere")), 3);
    TestEqual(TEXT("count == 0"), G.CountIn(TEXT("nowhere")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageMultipleGaragesTest,
    "GTC.Vehicles.Ownership.GarageStorage.MultipleGaragesIndependent",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageMultipleGaragesTest::RunTest(const FString& Parameters)
{
    GarageStorage G(2);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Store(TEXT("docks"), TEXT("boat1"));
    TestEqual(TEXT("car1 home"), G.GarageOf(TEXT("car1")), FString(TEXT("home")));
    TestEqual(TEXT("boat1 docks"), G.GarageOf(TEXT("boat1")), FString(TEXT("docks")));
    TestEqual(TEXT("total == 2"), G.TotalStored(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageImpoundFlagsTest,
    "GTC.Vehicles.Ownership.GarageStorage.ImpoundFlags",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageImpoundFlagsTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Impound(TEXT("car1"));
    TestTrue(TEXT("impounded"), G.IsImpounded(TEXT("car1")));
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageImpoundPullsTest,
    "GTC.Vehicles.Ownership.GarageStorage.ImpoundPullsFromGarage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageImpoundPullsTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Impound(TEXT("car1"));
    TestTrue(TEXT("impounded"), G.IsImpounded(TEXT("car1")));
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    TestEqual(TEXT("count == 0"), G.CountIn(TEXT("home")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageCannotStoreImpoundedTest,
    "GTC.Vehicles.Ownership.GarageStorage.CannotStoreImpounded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageCannotStoreImpoundedTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Impound(TEXT("car1"));
    const bool bOk = G.Store(TEXT("home"), TEXT("car1"));
    TestFalse(TEXT("store rejected"), bOk);
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    TestTrue(TEXT("still impounded"), G.IsImpounded(TEXT("car1")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRecoverPaysTest,
    "GTC.Vehicles.Ownership.GarageStorage.RecoverPaysFeeAndFrees",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRecoverPaysTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Impound(TEXT("car1"));
    const GarageStorage::FRecoverResult Result = G.RecoverFromImpound(TEXT("car1"), 1000, 250);
    TestTrue(TEXT("success"), Result.bSuccess);
    TestEqual(TEXT("cost == 250"), Result.Cost, 250);
    TestEqual(TEXT("new_balance == 750"), Result.NewBalance, 750);
    TestFalse(TEXT("not impounded"), G.IsImpounded(TEXT("car1")));
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRecoverBrokeTest,
    "GTC.Vehicles.Ownership.GarageStorage.RecoverFailsWhenBroke",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRecoverBrokeTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Impound(TEXT("car1"));
    const GarageStorage::FRecoverResult Result = G.RecoverFromImpound(TEXT("car1"), 100, 250);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("new_balance == 100"), Result.NewBalance, 100);
    TestTrue(TEXT("still impounded"), G.IsImpounded(TEXT("car1")));
    TestTrue(TEXT("reason mentions insufficient"), Result.Reason.Contains(TEXT("insufficient")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRecoverNotImpoundedTest,
    "GTC.Vehicles.Ownership.GarageStorage.RecoverNotImpoundedFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRecoverNotImpoundedTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    const GarageStorage::FRecoverResult Result = G.RecoverFromImpound(TEXT("car1"), 1000, 250);
    TestFalse(TEXT("not success"), Result.bSuccess);
    TestEqual(TEXT("new_balance == 1000"), Result.NewBalance, 1000);
    TestTrue(TEXT("reason mentions not impounded"), Result.Reason.Contains(TEXT("not impounded")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageRecoveredStoredAgainTest,
    "GTC.Vehicles.Ownership.GarageStorage.RecoveredCanBeStoredAgain",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageRecoveredStoredAgainTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Impound(TEXT("car1"));
    G.RecoverFromImpound(TEXT("car1"), 1000, 250);
    const bool bOk = G.Store(TEXT("home"), TEXT("car1"));
    TestTrue(TEXT("store ok"), bOk);
    TestTrue(TEXT("is_stored"), G.IsStored(TEXT("car1")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageTotalStoredTest,
    "GTC.Vehicles.Ownership.GarageStorage.TotalStored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageTotalStoredTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Store(TEXT("home"), TEXT("car2"));
    G.Store(TEXT("docks"), TEXT("boat1"));
    TestEqual(TEXT("total == 3"), G.TotalStored(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageSerializeRoundTripTest,
    "GTC.Vehicles.Ownership.GarageStorage.SerializeRestoreRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageSerializeRoundTripTest::RunTest(const FString& Parameters)
{
    GarageStorage G(3);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Store(TEXT("docks"), TEXT("boat1"));
    G.Impound(TEXT("wreck1"));

    int32 Cap = 0;
    TArray<TPair<FString, TArray<FString>>> Garages;
    TArray<FString> Impounded;
    G.Serialize(Cap, Garages, Impounded);

    GarageStorage G2(4);
    G2.Restore(Cap, Garages, Impounded);
    TestEqual(TEXT("capacity == 3"), G2.Capacity(), 3);
    TestEqual(TEXT("car1 home"), G2.GarageOf(TEXT("car1")), FString(TEXT("home")));
    TestEqual(TEXT("boat1 docks"), G2.GarageOf(TEXT("boat1")), FString(TEXT("docks")));
    TestTrue(TEXT("wreck1 impounded"), G2.IsImpounded(TEXT("wreck1")));
    TestEqual(TEXT("total == 2"), G2.TotalStored(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGarageStorageResetTest,
    "GTC.Vehicles.Ownership.GarageStorage.ResetClearsEverything",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGarageStorageResetTest::RunTest(const FString& Parameters)
{
    GarageStorage G(4);
    G.Store(TEXT("home"), TEXT("car1"));
    G.Impound(TEXT("wreck1"));
    G.Reset();
    TestEqual(TEXT("total == 0"), G.TotalStored(), 0);
    TestFalse(TEXT("not impounded"), G.IsImpounded(TEXT("wreck1")));
    TestFalse(TEXT("not stored"), G.IsStored(TEXT("car1")));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
