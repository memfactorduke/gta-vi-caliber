// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../DistrictEconomy.h"
#include "../../Faction/GangTerritory.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_district_economy.gd. Float compares use Eps. Includes a
// GangTerritory composition test (turf influence feeds district desirability).

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistDefaultLoadedTest,
    "GTC.Systems.Economy.DistrictEconomy.DefaultDistrictsLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistDefaultLoadedTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    TestEqual(TEXT("count == 4"), E.DistrictCount(), 4);
    TestTrue(TEXT("has downtown"), E.HasDistrict(TEXT("downtown")));
    TestTrue(TEXT("has beach"), E.HasDistrict(TEXT("beach")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistMalformedDroppedTest,
    "GTC.Systems.Economy.DistrictEconomy.MalformedDistrictsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistMalformedDroppedTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E({
        FDistrictDef(TEXT("ok"), 1.0),
        FDistrictDef(TEXT(""), 1.0),
        FDistrictDef(TEXT("free"), 0.0),
        FDistrictDef(TEXT("ok"), 2.0),
    });
    TestEqual(TEXT("count == 1"), E.DistrictCount(), 1);
    TestTrue(TEXT("has ok"), E.HasDistrict(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistBaseIndexTest,
    "GTC.Systems.Economy.DistrictEconomy.BaseIndexLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistBaseIndexTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    TestEqual(TEXT("beach == 1.4"), E.BaseIndex(TEXT("beach")), 1.4, Eps);
    TestEqual(TEXT("nope == -1"), E.BaseIndex(TEXT("nope")), -1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistNeutralDesirTest,
    "GTC.Systems.Economy.DistrictEconomy.NeutralDesirabilityIsBase",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistNeutralDesirTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    TestEqual(TEXT("downtown == 1.2"), E.Desirability(TEXT("downtown")), 1.2, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistUnknownDesirTest,
    "GTC.Systems.Economy.DistrictEconomy.UnknownDesirabilityIsNeutral",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistUnknownDesirTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    TestEqual(TEXT("nope == 1.0"), E.Desirability(TEXT("nope")), 1.0, Eps);
    TestEqual(TEXT("property_value 1000"), E.PropertyValue(1000, TEXT("nope")), 1000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistControlRaisesTest,
    "GTC.Systems.Economy.DistrictEconomy.ControlRaisesDesirability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistControlRaisesTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.SetControl(TEXT("downtown"), 1.0);
    TestEqual(TEXT("downtown == 1.5"), E.Desirability(TEXT("downtown")), 1.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistControlClampedTest,
    "GTC.Systems.Economy.DistrictEconomy.ControlClamped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistControlClampedTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.SetControl(TEXT("downtown"), 5.0);
    TestEqual(TEXT("control == 1.0"), E.ControlIn(TEXT("downtown")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistHeatLowersTest,
    "GTC.Systems.Economy.DistrictEconomy.HeatLowersDesirability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistHeatLowersTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.AddHeat(TEXT("downtown"), 0.5);
    TestEqual(TEXT("downtown == 1.0"), E.Desirability(TEXT("downtown")), 1.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistInvestRaisesTest,
    "GTC.Systems.Economy.DistrictEconomy.InvestmentRaisesAndCaps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistInvestRaisesTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    for (int32 I = 0; I < 5; ++I)
    {
        E.Invest(TEXT("downtown"));
    }
    TestEqual(TEXT("investment == 5"), E.InvestmentIn(TEXT("downtown")), 5);
    TestEqual(TEXT("downtown == 1.5"), E.Desirability(TEXT("downtown")), 1.5, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistDivestFloorsTest,
    "GTC.Systems.Economy.DistrictEconomy.DivestFloorsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistDivestFloorsTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.Invest(TEXT("docks"));
    E.Divest(TEXT("docks"));
    E.Divest(TEXT("docks"));
    TestEqual(TEXT("investment == 0"), E.InvestmentIn(TEXT("docks")), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistClampsHighTest,
    "GTC.Systems.Economy.DistrictEconomy.DesirabilityClampsHigh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistClampsHighTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.SetControl(TEXT("beach"), 1.0);
    for (int32 I = 0; I < 3; ++I)
    {
        E.Invest(TEXT("beach"));
    }
    TestEqual(TEXT("beach == DesirMax"), E.Desirability(TEXT("beach")), DistrictEconomy::DesirMax, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistClampsLowTest,
    "GTC.Systems.Economy.DistrictEconomy.DesirabilityClampsLow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistClampsLowTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.AddHeat(TEXT("docks"), 1.0);
    TestEqual(TEXT("docks == DesirMin"), E.Desirability(TEXT("docks")), DistrictEconomy::DesirMin, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistPropertyValueTest,
    "GTC.Systems.Economy.DistrictEconomy.PropertyValueScales",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistPropertyValueTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    TestEqual(TEXT("100000 * 1.2 == 120000"), E.PropertyValue(100000, TEXT("downtown")), 120000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistIncomeMultTest,
    "GTC.Systems.Economy.DistrictEconomy.IncomeMultiplierTracksDesirability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistIncomeMultTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.SetControl(TEXT("beach"), 1.0);
    TestEqual(TEXT("income == desirability"), E.IncomeMultiplier(TEXT("beach")), E.Desirability(TEXT("beach")), Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistDecayFloorsTest,
    "GTC.Systems.Economy.DistrictEconomy.DecayHeatFloors",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistDecayFloorsTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.AddHeat(TEXT("downtown"), 0.3);
    E.DecayHeat(TEXT("downtown"), 0.5);
    TestEqual(TEXT("heat == 0"), E.HeatIn(TEXT("downtown")), 0.0, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistDecayAllTest,
    "GTC.Systems.Economy.DistrictEconomy.DecayAllHeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistDecayAllTest::RunTest(const FString& Parameters)
{
    DistrictEconomy E;
    E.AddHeat(TEXT("downtown"), 0.5);
    E.AddHeat(TEXT("beach"), 0.5);
    E.DecayAllHeat(0.2);
    TestEqual(TEXT("downtown heat == 0.3"), E.HeatIn(TEXT("downtown")), 0.3, Eps);
    TestEqual(TEXT("beach heat == 0.3"), E.HeatIn(TEXT("beach")), 0.3, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FDistGangInfluenceTest,
    "GTC.Systems.Economy.DistrictEconomy.GangInfluenceDrivesDesirability",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FDistGangInfluenceTest::RunTest(const FString& Parameters)
{
    // Composition: taking turf (GangTerritory influence) gentrifies the district.
    GangTerritory GT;
    GT.AddInfluence(TEXT("downtown"), 1.0);
    DistrictEconomy E;
    const double Before = E.Desirability(TEXT("downtown"));
    E.SetControl(TEXT("downtown"), GT.InfluenceIn(TEXT("downtown")));
    TestTrue(TEXT("desirability rose"), E.Desirability(TEXT("downtown")) > Before);
    TestTrue(TEXT("property value rose"), E.PropertyValue(100000, TEXT("downtown")) > 120000);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
