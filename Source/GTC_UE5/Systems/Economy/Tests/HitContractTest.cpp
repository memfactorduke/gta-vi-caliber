// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../HitContract.h"
#include "../StockMarket.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_hit_contract.gd. Includes a cross-system test proving the
// signature loop (a completed hit's market_effect fed to StockMarket).

namespace
{
    FHitContractDef MakeOk(const FString& Id, int32 Reward)
    {
        FHitContractDef Def;
        Def.Id = Id;
        Def.Reward = Reward;
        return Def;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitDefaultBoardTest,
    "GTC.Systems.Economy.HitContract.DefaultBoardLoaded",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitDefaultBoardTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    TestEqual(TEXT("count == 3"), HC.ContractCount(), 3);
    TestTrue(TEXT("has tech_takedown"), HC.HasContract(TEXT("tech_takedown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitMalformedDroppedTest,
    "GTC.Systems.Economy.HitContract.MalformedContractsDropped",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitMalformedDroppedTest::RunTest(const FString& Parameters)
{
    HitContract HC({
        MakeOk(TEXT("ok"), 1000),
        MakeOk(TEXT(""), 1000),
        MakeOk(TEXT("free"), 0),
        MakeOk(TEXT("ok"), 9999),
    });
    TestEqual(TEXT("count == 1"), HC.ContractCount(), 1);
    TestTrue(TEXT("has ok"), HC.HasContract(TEXT("ok")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitRewardLookupTest,
    "GTC.Systems.Economy.HitContract.RewardLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitRewardLookupTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    TestEqual(TEXT("airline_war 18000"), HC.RewardOf(TEXT("airline_war")), 18000);
    TestEqual(TEXT("nope -1"), HC.RewardOf(TEXT("nope")), -1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitTargetLookupTest,
    "GTC.Systems.Economy.HitContract.TargetLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitTargetLookupTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    TestEqual(TEXT("airline_war target"), HC.TargetOf(TEXT("airline_war")), FString(TEXT("Don Percival")));
    TestEqual(TEXT("nope empty"), HC.TargetOf(TEXT("nope")), FString());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitMarketEffectTest,
    "GTC.Systems.Economy.HitContract.MarketEffectLookup",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitMarketEffectTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    const FMarketEffect E = HC.MarketEffectOf(TEXT("airline_war"));
    TestEqual(TEXT("company"), E.CompanyId, FString(TEXT("pelican_air")));
    TestEqual(TEXT("magnitude -0.4"), E.Magnitude, -0.4, Eps);
    TestFalse(TEXT("nope invalid"), HC.MarketEffectOf(TEXT("nope")).bValid);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAllAvailableTest,
    "GTC.Systems.Economy.HitContract.AllAvailableInitially",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAllAvailableTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    TestEqual(TEXT("available 3"), HC.Available().Num(), 3);
    TestFalse(TEXT("no active"), HC.HasActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAcceptTest,
    "GTC.Systems.Economy.HitContract.AcceptSetsActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAcceptTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    TestTrue(TEXT("accept"), HC.Accept(TEXT("tech_takedown")));
    TestEqual(TEXT("active"), HC.Active(), FString(TEXT("tech_takedown")));
    TestTrue(TEXT("has active"), HC.HasActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAcceptUnknownTest,
    "GTC.Systems.Economy.HitContract.AcceptUnknownFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAcceptUnknownTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    TestFalse(TEXT("not accept"), HC.Accept(TEXT("nope")));
    TestFalse(TEXT("no active"), HC.HasActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAcceptWhileActiveTest,
    "GTC.Systems.Economy.HitContract.AcceptWhileActiveFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAcceptWhileActiveTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    TestFalse(TEXT("second accept fails"), HC.Accept(TEXT("airline_war")));
    TestEqual(TEXT("still tech"), HC.Active(), FString(TEXT("tech_takedown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAvailableExcludesActiveTest,
    "GTC.Systems.Economy.HitContract.AvailableExcludesActive",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAvailableExcludesActiveTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    TestFalse(TEXT("not in available"), HC.Available().Contains(TEXT("tech_takedown")));
    TestEqual(TEXT("available 2"), HC.Available().Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAbandonTest,
    "GTC.Systems.Economy.HitContract.AbandonReturnsToPool",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAbandonTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    const FString Dropped = HC.Abandon();
    TestEqual(TEXT("dropped"), Dropped, FString(TEXT("tech_takedown")));
    TestFalse(TEXT("no active"), HC.HasActive());
    TestTrue(TEXT("back in pool"), HC.Available().Contains(TEXT("tech_takedown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitCompleteBanksTest,
    "GTC.Systems.Economy.HitContract.CompleteBanksRewardAndMarksDone",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitCompleteBanksTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    const FHitCompleteResult R = HC.Complete();
    TestTrue(TEXT("success"), R.bSuccess);
    TestEqual(TEXT("reward 25000"), R.Reward, 25000);
    TestTrue(TEXT("completed"), HC.IsCompleted(TEXT("tech_takedown")));
    TestFalse(TEXT("no active"), HC.HasActive());
    TestEqual(TEXT("total 25000"), HC.TotalEarned(), 25000);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitCompleteEffectTest,
    "GTC.Systems.Economy.HitContract.CompleteReturnsMarketEffect",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitCompleteEffectTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("airline_war"));
    const FMarketEffect Effect = HC.Complete().MarketEffect;
    TestEqual(TEXT("company"), Effect.CompanyId, FString(TEXT("pelican_air")));
    TestEqual(TEXT("magnitude -0.4"), Effect.Magnitude, -0.4, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitCompleteNoActiveTest,
    "GTC.Systems.Economy.HitContract.CompleteNoActiveFails",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitCompleteNoActiveTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    const FHitCompleteResult R = HC.Complete();
    TestFalse(TEXT("not success"), R.bSuccess);
    TestEqual(TEXT("reward 0"), R.Reward, 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitCannotReacceptTest,
    "GTC.Systems.Economy.HitContract.CannotReacceptCompleted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitCannotReacceptTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    HC.Complete();
    TestFalse(TEXT("cannot reaccept"), HC.Accept(TEXT("tech_takedown")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitAvailableExcludesCompletedTest,
    "GTC.Systems.Economy.HitContract.AvailableExcludesCompleted",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitAvailableExcludesCompletedTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    HC.Complete();
    TestFalse(TEXT("not in available"), HC.Available().Contains(TEXT("tech_takedown")));
    TestEqual(TEXT("available 2"), HC.Available().Num(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitTotalEarnedTest,
    "GTC.Systems.Economy.HitContract.TotalEarnedAccumulates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitTotalEarnedTest::RunTest(const FString& Parameters)
{
    HitContract HC;
    HC.Accept(TEXT("tech_takedown"));
    HC.Complete();
    HC.Accept(TEXT("airline_war"));
    HC.Complete();
    TestEqual(TEXT("total 43000"), HC.TotalEarned(), 43000);
    TestEqual(TEXT("completed 2"), HC.CompletedCount(), 2);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHitMovesStockTest,
    "GTC.Systems.Economy.HitContract.HitMovesStockMarket",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FHitMovesStockTest::RunTest(const FString& Parameters)
{
    // The signature loop: complete a hit, apply its effect to the live market.
    HitContract HC;
    StockMarket SM;
    const int32 BasePelican = SM.Price(TEXT("pelican_air"));
    const int32 BaseAugury = SM.Price(TEXT("augury_air"));
    HC.Accept(TEXT("airline_war"));
    const FMarketEffect Effect = HC.Complete().MarketEffect;
    SM.ApplyRivalryShock(Effect.CompanyId, Effect.Magnitude, Effect.Spillover);
    TestTrue(TEXT("pelican fell"), SM.Price(TEXT("pelican_air")) < BasePelican);
    TestTrue(TEXT("augury rose"), SM.Price(TEXT("augury_air")) > BaseAugury);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
