// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../ChopShop.h"
#include "../VehicleHealth.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_chop_shop.gd (15 tests). One test composes FVehicleHealth (a damaged car
// fences for less), mirroring the cross-ref oracle.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopDefaultClassesLoadedTest,
    "GTC.Vehicles.ChopShop.DefaultClassesLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopDefaultClassesLoadedTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    TestEqual(TEXT("class_count == 7"), C.ClassCount(), 7);
    TestTrue(TEXT("has sports"), C.HasClass(TEXT("sports")));
    TestTrue(TEXT("has super"), C.HasClass(TEXT("super")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopMalformedClassesDroppedTest,
    "GTC.Vehicles.ChopShop.MalformedClassesDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopMalformedClassesDroppedTest::RunTest(const FString& Parameters)
{
    TArray<FChopShop::FClassDef> Defs;
    Defs.Add({TEXT("ok"), 1000});
    Defs.Add({TEXT(""), 1000});      // empty id
    Defs.Add({TEXT("no_base"), 0});  // no base (modeled as base 0 -> dropped)
    Defs.Add({TEXT("free"), 0});     // non-positive base
    Defs.Add({TEXT("ok"), 2000});    // duplicate id
    FChopShop C(Defs);
    TestEqual(TEXT("class_count == 1"), C.ClassCount(), 1);
    TestTrue(TEXT("has ok"), C.HasClass(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopBaseValueLookupTest,
    "GTC.Vehicles.ChopShop.BaseValueLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopBaseValueLookupTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    TestEqual(TEXT("sports base == 60000"), C.BaseValueOf(TEXT("sports")), 60000);
    TestEqual(TEXT("nope base == -1"), C.BaseValueOf(TEXT("nope")), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopPristineValueIsBaseTest,
    "GTC.Vehicles.ChopShop.PristineValueIsBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopPristineValueIsBaseTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    TestEqual(TEXT("value sports 1.0 == 60000"), C.Value(TEXT("sports"), 1.0), 60000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopWreckedValueIsScrapFloorTest,
    "GTC.Vehicles.ChopShop.WreckedValueIsScrapFloor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopWreckedValueIsScrapFloorTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    // 60000 * 0.2 = 12000
    TestEqual(TEXT("value sports 0.0 == 12000"), C.Value(TEXT("sports"), 0.0), 12000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopMidConditionValueTest,
    "GTC.Vehicles.ChopShop.MidConditionValue",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopMidConditionValueTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    // 60000 * (0.2 + 0.8*0.5) = 60000 * 0.6 = 36000
    TestEqual(TEXT("value sports 0.5 == 36000"), C.Value(TEXT("sports"), 0.5), 36000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopValueUnknownIsZeroTest,
    "GTC.Vehicles.ChopShop.ValueUnknownIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopValueUnknownIsZeroTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    TestEqual(TEXT("value nope == 0"), C.Value(TEXT("nope"), 1.0), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopRequestAppliesDemandBonusTest,
    "GTC.Vehicles.ChopShop.RequestAppliesDemandBonus",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopRequestAppliesDemandBonusTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    C.SetRequests({TEXT("sports")});
    // 60000 * 1.0 * 1.5 = 90000
    TestTrue(TEXT("is requested"), C.IsRequested(TEXT("sports")));
    TestEqual(TEXT("value == 90000"), C.Value(TEXT("sports"), 1.0), 90000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopSetRequestsIgnoresUnknownTest,
    "GTC.Vehicles.ChopShop.SetRequestsIgnoresUnknown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopSetRequestsIgnoresUnknownTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    C.SetRequests({TEXT("sports"), TEXT("spaceship")});
    TestTrue(TEXT("is requested sports"), C.IsRequested(TEXT("sports")));
    TestFalse(TEXT("not requested spaceship"), C.IsRequested(TEXT("spaceship")));
    TestEqual(TEXT("requested size == 1"), C.Requested().Num(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopDeliverPaysAndBanksTest,
    "GTC.Vehicles.ChopShop.DeliverPaysAndBanks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopDeliverPaysAndBanksTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    const FChopShop::FDeliverResult R = C.Deliver(TEXT("muscle"), 1.0);
    TestTrue(TEXT("accepted"), R.bAccepted);
    TestEqual(TEXT("payout == 35000"), R.Payout, 35000);
    TestEqual(TEXT("total_earned == 35000"), C.TotalEarned(), 35000);
    TestEqual(TEXT("deliveries == 1"), C.DeliveriesCount(), 1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopDeliverUnknownRejectedTest,
    "GTC.Vehicles.ChopShop.DeliverUnknownRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopDeliverUnknownRejectedTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    const FChopShop::FDeliverResult R = C.Deliver(TEXT("nope"), 1.0);
    TestFalse(TEXT("not accepted"), R.bAccepted);
    TestEqual(TEXT("payout == 0"), R.Payout, 0);
    TestEqual(TEXT("total_earned == 0"), C.TotalEarned(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopHotVehicleDiscountedTest,
    "GTC.Vehicles.ChopShop.HotVehicleDiscounted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopHotVehicleDiscountedTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    const FChopShop::FDeliverResult R = C.Deliver(TEXT("sports"), 1.0, true);
    // 60000 * (1 - 0.25) = 45000
    TestEqual(TEXT("payout == 45000"), R.Payout, 45000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopDeliveryFulfilsRequestTest,
    "GTC.Vehicles.ChopShop.DeliveryFulfilsRequest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopDeliveryFulfilsRequestTest::RunTest(const FString& Parameters)
{
    FChopShop C;
    C.SetRequests({TEXT("sports")});
    const FChopShop::FDeliverResult First = C.Deliver(TEXT("sports"), 1.0);  // 90000, was requested
    const FChopShop::FDeliverResult Second = C.Deliver(TEXT("sports"), 1.0); // base 60000, fulfilled
    TestTrue(TEXT("first was_requested"), First.bWasRequested);
    TestEqual(TEXT("first payout == 90000"), First.Payout, 90000);
    TestFalse(TEXT("no longer requested"), C.IsRequested(TEXT("sports")));
    TestEqual(TEXT("second payout == 60000"), Second.Payout, 60000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopRotateRequestsDeterministicTest,
    "GTC.Vehicles.ChopShop.RotateRequestsDeterministic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopRotateRequestsDeterministicTest::RunTest(const FString& Parameters)
{
    // RNG parity: FRandomStream is deterministic per seed (NOT byte-identical to Godot's
    // RandomNumberGenerator); the oracle only pins determinism + count, which holds.
    FChopShop A;
    FChopShop B;
    FRandomStream RngA(4);
    FRandomStream RngB(4);
    A.RotateRequests(RngA, 2);
    B.RotateRequests(RngB, 2);
    TestEqual(TEXT("requested size == 2"), A.Requested().Num(), 2);
    TestTrue(TEXT("a == b (same seed)"), A.Requested() == B.Requested());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FChopShopDamagedCarFencesForLessTest,
    "GTC.Vehicles.ChopShop.DamagedCarFencesForLess",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FChopShopDamagedCarFencesForLessTest::RunTest(const FString& Parameters)
{
    // Composition: FVehicleHealth::HealthFraction feeds the condition.
    FVehicleHealth VH(1000.0);
    const int32 Pristine = FChopShop().Value(TEXT("muscle"), VH.HealthFraction());
    VH.ApplyDamage(500.0); // half health
    const int32 Damaged = FChopShop().Value(TEXT("muscle"), VH.HealthFraction());
    TestEqual(TEXT("pristine == 35000"), Pristine, 35000);
    TestTrue(TEXT("damaged < pristine"), Damaged < Pristine);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
