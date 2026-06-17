// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PlayerStats.h"
#include "../../../Tests/GtcTestTolerances.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

#include "PlayerStatsTestListener.h"

using GtcTest::Eps;

// ============================================================================
// PARITY tests — each maps 1:1 to a func in the Godot oracle
// game/tests/unit/test_player_stats.gd (9 funcs). Identical literals/tolerances.
// ============================================================================

// --- test_absorb_full_soak ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsAbsorbFullSoakTest,
    "GTC.Player.Stats.AbsorbFullSoak",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsAbsorbFullSoakTest::RunTest(const FString& Parameters)
{
    double Remaining = 0.0;
    double Overflow = 0.0;
    FPlayerStats::Absorb(100.0, 30.0, Remaining, Overflow);
    TestTrue(TEXT("remaining == 70"), FMath::Abs(Remaining - 70.0) < Eps);
    TestTrue(TEXT("overflow == 0"), FMath::Abs(Overflow) < Eps);
    return true;
}

// --- test_absorb_overflow ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsAbsorbOverflowTest,
    "GTC.Player.Stats.AbsorbOverflow",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsAbsorbOverflowTest::RunTest(const FString& Parameters)
{
    double Remaining = 0.0;
    double Overflow = 0.0;
    FPlayerStats::Absorb(20.0, 50.0, Remaining, Overflow);
    TestTrue(TEXT("remaining == 0"), FMath::Abs(Remaining) < Eps);
    TestTrue(TEXT("overflow == 30"), FMath::Abs(Overflow - 30.0) < Eps);
    return true;
}

// --- test_absorb_no_armor ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsAbsorbNoArmorTest,
    "GTC.Player.Stats.AbsorbNoArmor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsAbsorbNoArmorTest::RunTest(const FString& Parameters)
{
    double Remaining = 0.0;
    double Overflow = 0.0;
    FPlayerStats::Absorb(0.0, 40.0, Remaining, Overflow);
    TestTrue(TEXT("remaining == 0"), FMath::Abs(Remaining) < Eps);
    TestTrue(TEXT("overflow == 40"), FMath::Abs(Overflow - 40.0) < Eps);
    return true;
}

// --- test_absorb_negative_damage_safe ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsAbsorbNegativeDamageSafeTest,
    "GTC.Player.Stats.AbsorbNegativeDamageSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsAbsorbNegativeDamageSafeTest::RunTest(const FString& Parameters)
{
    double Remaining = 0.0;
    double Overflow = 0.0;
    FPlayerStats::Absorb(50.0, -10.0, Remaining, Overflow);
    TestTrue(TEXT("remaining == 50"), FMath::Abs(Remaining - 50.0) < Eps);
    TestTrue(TEXT("overflow == 0"), FMath::Abs(Overflow) < Eps);
    return true;
}

// --- test_fraction_normal ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsFractionNormalTest,
    "GTC.Player.Stats.FractionNormal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsFractionNormalTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("25/100 == 0.25"), FMath::Abs(FPlayerStats::Fraction(25.0, 100.0) - 0.25) < Eps);
    return true;
}

// --- test_fraction_clamps_high ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsFractionClampsHighTest,
    "GTC.Player.Stats.FractionClampsHigh",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsFractionClampsHighTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("150/100 clamps to 1"), FMath::Abs(FPlayerStats::Fraction(150.0, 100.0) - 1.0) < Eps);
    return true;
}

// --- test_fraction_zero_max_safe ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsFractionZeroMaxSafeTest,
    "GTC.Player.Stats.FractionZeroMaxSafe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsFractionZeroMaxSafeTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("5/0 safe -> 0"), FMath::Abs(FPlayerStats::Fraction(5.0, 0.0)) < Eps);
    return true;
}

// --- test_save_round_trip_restores_wallet_and_armor ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsSaveRoundTripTest,
    "GTC.Player.Stats.SaveRoundTripRestoresWalletAndArmor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsSaveRoundTripTest::RunTest(const FString& Parameters)
{
    FPlayerStats Stats;
    Stats.Money = 4250;
    Stats.Armor = 62.5;
    int32 SnapMoney = 0;
    double SnapArmor = 0.0;
    Stats.Serialize(SnapMoney, SnapArmor);

    FPlayerStats Fresh;
    Fresh.Money = 0;
    Fresh.Armor = 0.0;
    // Snapshot fields are numeric -> both valid.
    Fresh.Restore(true, static_cast<double>(SnapMoney), true, SnapArmor);

    TestEqual(TEXT("money restored"), Fresh.Money, 4250);
    TestTrue(TEXT("armor restored"), FMath::Abs(Fresh.Armor - 62.5) < Eps);
    return true;
}

// --- test_restore_clamps_and_survives_garbage ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsRestoreClampsGarbageTest,
    "GTC.Player.Stats.RestoreClampsAndSurvivesGarbage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsRestoreClampsGarbageTest::RunTest(const FString& Parameters)
{
    FPlayerStats Stats;
    Stats.Money = 700;
    Stats.Armor = 10.0;

    // restore({"money": -50, "armor": 9999.0}) — numeric, so clamped.
    Stats.Restore(true, -50.0, true, 9999.0);
    TestEqual(TEXT("money floored to 0"), Stats.Money, 0);
    TestTrue(TEXT("armor clamped to max"), FMath::Abs(Stats.Armor - Stats.MaxArmor) < Eps);

    // restore({"money": "junk", "armor": null}) — non-numeric, so kept (invalid).
    Stats.Restore(false, 0.0, false, 0.0);
    TestEqual(TEXT("money kept (still 0)"), Stats.Money, 0);
    TestTrue(TEXT("armor kept (still max)"), FMath::Abs(Stats.Armor - Stats.MaxArmor) < Eps);
    return true;
}

// ============================================================================
// COMPONENT-BEHAVIOR tests — ownership / lifecycle / delegate firing.
// These layer thin checks on top of the 9 parity funcs (not part of the count).
// ============================================================================

// Pure-data mutator parity that the oracle leaves untested (Godot mutators need a
// SceneTree). Drives FPlayerStats directly — no engine context.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsMutatorSemanticsTest,
    "GTC.Player.Stats.MutatorSemantics",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsMutatorSemanticsTest::RunTest(const FString& Parameters)
{
    FPlayerStats S;
    S.InitDefaults();
    TestEqual(TEXT("wallet seeded to starting_money"), S.Money, 1500);
    TestEqual(TEXT("objective seeded"), S.ObjectiveTitle, FString(TEXT("Explore the city")));

    // add_armor ignores negatives, clamps to max.
    S.AddArmor(30.0);
    TestTrue(TEXT("armor 30"), FMath::Abs(S.Armor - 30.0) < Eps);
    S.AddArmor(-5.0);
    TestTrue(TEXT("negative armor ignored"), FMath::Abs(S.Armor - 30.0) < Eps);
    S.AddArmor(1000.0);
    TestTrue(TEXT("armor clamps to max"), FMath::Abs(S.Armor - S.MaxArmor) < Eps);

    // soak_damage drains armor, returns overflow.
    bool bChanged = false;
    const double Overflow = S.SoakDamage(120.0, bChanged);
    TestTrue(TEXT("soak changed armor"), bChanged);
    TestTrue(TEXT("armor drained to 0"), FMath::Abs(S.Armor) < Eps);
    TestTrue(TEXT("overflow 20"), FMath::Abs(Overflow - 20.0) < Eps);

    // soak with non-positive damage: no change, overflow floored at 0.
    bool bChanged2 = true;
    const double Overflow2 = S.SoakDamage(-5.0, bChanged2);
    TestFalse(TEXT("no armor change on non-positive"), bChanged2);
    TestTrue(TEXT("overflow floored 0"), FMath::Abs(Overflow2) < Eps);

    // spend_money guards affordability.
    TestFalse(TEXT("cannot spend more than wallet"), S.SpendMoney(99999));
    TestFalse(TEXT("cannot spend non-positive"), S.SpendMoney(0));
    TestTrue(TEXT("affordable spend succeeds"), S.SpendMoney(500));
    TestEqual(TEXT("wallet debited"), S.Money, 1000);

    // add_money floors at zero.
    S.AddMoney(-99999);
    TestEqual(TEXT("wallet floored at 0"), S.Money, 0);

    // objective set/clear + waypoint flag.
    S.SetObjective(TEXT("Reach the docks"), FVector(1, 2, 3), true);
    TestTrue(TEXT("has waypoint"), S.HasWaypoint());
    S.ClearObjective();
    TestFalse(TEXT("waypoint cleared"), S.HasWaypoint());
    TestEqual(TEXT("title cleared"), S.ObjectiveTitle, FString());
    return true;
}

// Lifecycle + delegate firing on a real spawned actor in a transient world.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerStatsComponentLifecycleTest,
    "GTC.Player.Stats.ComponentLifecycleAndDelegates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerStatsComponentLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("transient world created"), World))
    {
        return false;
    }
    FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
    Ctx.SetCurrentWorld(World);

    AActor* Owner = World->SpawnActor<AActor>();
    UPlayerStatsComponent* Comp = NewObject<UPlayerStatsComponent>(Owner);

    // Capture delegate fires via a real UObject listener (dynamic delegates only
    // bind to UFUNCTIONs). Bind BEFORE RegisterComponent so BeginPlay's announce
    // is observed.
    UPlayerStatsTestListener* Listener = NewObject<UPlayerStatsTestListener>(Owner);
    Comp->OnArmorChanged.AddDynamic(Listener, &UPlayerStatsTestListener::HandleArmor);
    Comp->OnMoneyChanged.AddDynamic(Listener, &UPlayerStatsTestListener::HandleMoney);
    Comp->OnObjectiveChanged.AddDynamic(Listener, &UPlayerStatsTestListener::HandleObjective);

    Comp->RegisterComponent();

    // BeginPlay seeds defaults and announces money + objective.
    World->InitializeActorsForPlay(FURL());
    Owner->DispatchBeginPlay();

    TestEqual(TEXT("BeginPlay seeded wallet"), Comp->GetMoney(), 1500);
    TestTrue(TEXT("money announced on init"), Listener->MoneyFires >= 1);
    TestTrue(TEXT("objective announced on init"), Listener->ObjectiveFires >= 1);

    // Mutators fire the matching delegate.
    Comp->AddArmor(25.0f);
    TestTrue(TEXT("armor delegate fired"), Listener->ArmorFires >= 1);
    TestTrue(TEXT("armor accessor updated"), FMath::Abs(Comp->GetArmor() - 25.0f) < Eps);
    TestTrue(TEXT("armor delegate payload"), FMath::Abs(Listener->LastArmor - 25.0f) < Eps);

    const int32 MoneyFiresBefore = Listener->MoneyFires;
    Comp->AddMoney(250);
    TestEqual(TEXT("money delegate fired once"), Listener->MoneyFires, MoneyFiresBefore + 1);
    TestEqual(TEXT("money payload"), Listener->LastMoney, 1750);

    // Failed spend must NOT broadcast.
    const int32 FiresBeforeFailedSpend = Listener->MoneyFires;
    TestFalse(TEXT("unaffordable spend fails"), Comp->SpendMoney(999999));
    TestEqual(TEXT("no broadcast on failed spend"), Listener->MoneyFires, FiresBeforeFailedSpend);

    // Cleanup.
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
