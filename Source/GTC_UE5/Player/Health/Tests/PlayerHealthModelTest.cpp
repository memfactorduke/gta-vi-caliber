// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PlayerHealthModel.h"
#include "../PlayerHealthComponent.h"
#include "../../../Tests/GtcTestTolerances.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

#include "PlayerHealthTestListener.h"

using GtcTest::Eps;

// ============================================================================
// PARITY tests — each maps 1:1 to a func in the Godot oracle
// game/tests/unit/test_player_health_model.gd (18 funcs). Identical literals
// and tolerances; is_equal_approx(...) -> FMath::Abs(diff) < Eps. Compound
// boolean returns are split into independent assertions.
// ============================================================================

// --- test_starts_full ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthStartsFullTest,
    "GTC.Player.Health.StartsFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthStartsFullTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    TestTrue(TEXT("health == 100"), FMath::Abs(H.Health - 100.0) < Eps);
    TestFalse(TEXT("not dead"), H.IsDead());
    return true;
}

// --- test_damage_reduces_health ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthDamageReducesTest,
    "GTC.Player.Health.DamageReducesHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthDamageReducesTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    H.Apply(30.0);
    TestTrue(TEXT("health == 70"), FMath::Abs(H.Health - 70.0) < Eps);
    return true;
}

// --- test_negative_damage_ignored ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthNegativeDamageIgnoredTest,
    "GTC.Player.Health.NegativeDamageIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthNegativeDamageIgnoredTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    H.Apply(-20.0);
    TestTrue(TEXT("health == 100"), FMath::Abs(H.Health - 100.0) < Eps);
    return true;
}

// --- test_death_on_lethal_hit ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthDeathOnLethalHitTest,
    "GTC.Player.Health.DeathOnLethalHit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthDeathOnLethalHitTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(50.0);
    const bool bKilled = H.Apply(60.0);
    TestTrue(TEXT("apply returns true on lethal"), bKilled);
    TestTrue(TEXT("is dead"), H.IsDead());
    TestTrue(TEXT("health == 0"), FMath::Abs(H.Health) < Eps);
    return true;
}

// --- test_no_regen_during_delay ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthNoRegenDuringDelayTest,
    "GTC.Player.Health.NoRegenDuringDelay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthNoRegenDuringDelayTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0, 10.0, 5.0);
    H.Apply(40.0);
    H.Tick(2.0); // within the 5s delay
    TestTrue(TEXT("health == 60 (no regen)"), FMath::Abs(H.Health - 60.0) < Eps);
    return true;
}

// --- test_regen_after_delay ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthRegenAfterDelayTest,
    "GTC.Player.Health.RegenAfterDelay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthRegenAfterDelayTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0, 10.0, 5.0);
    H.Apply(40.0);
    H.Tick(5.0); // reaches the delay; the crossing frame regenerates ~nothing
    H.Tick(1.0); // a 1s frame past the threshold regenerates 10
    TestTrue(TEXT("health > 60 after regen"), H.Health > 60.0);
    return true;
}

// --- test_regen_caps_at_max ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthRegenCapsAtMaxTest,
    "GTC.Player.Health.RegenCapsAtMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthRegenCapsAtMaxTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0, 10.0, 0.0);
    H.Apply(5.0);
    for (int32 I = 0; I < 100; ++I)
    {
        H.Tick(1.0);
    }
    TestTrue(TEXT("health caps at 100"), FMath::Abs(H.Health - 100.0) < Eps);
    return true;
}

// --- test_damage_resets_regen_timer ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthDamageResetsTimerTest,
    "GTC.Player.Health.DamageResetsRegenTimer",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthDamageResetsTimerTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0, 10.0, 5.0);
    H.Apply(40.0);
    H.Tick(4.9);  // almost at delay
    H.Apply(0.0); // a graze resets the timer
    H.Tick(1.0);  // would-be-regen frame, but timer reset -> still no regen
    TestTrue(TEXT("health == 60 (timer reset)"), FMath::Abs(H.Health - 60.0) < Eps);
    return true;
}

// --- test_dead_does_not_regen ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthDeadDoesNotRegenTest,
    "GTC.Player.Health.DeadDoesNotRegen",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthDeadDoesNotRegenTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(50.0, 10.0, 0.0);
    H.Apply(50.0);
    H.Tick(5.0);
    TestTrue(TEXT("is dead"), H.IsDead());
    TestTrue(TEXT("health == 0"), FMath::Abs(H.Health) < Eps);
    return true;
}

// --- test_fraction ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthFractionTest,
    "GTC.Player.Health.Fraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthFractionTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(80.0);
    H.Apply(20.0);
    TestTrue(TEXT("fraction == 0.75"), FMath::Abs(H.Fraction() - 0.75) < Eps);
    return true;
}

// --- test_heal_restores_capped_at_max ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthHealRestoresCappedTest,
    "GTC.Player.Health.HealRestoresCappedAtMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthHealRestoresCappedTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    H.Apply(60.0);
    H.Heal(30.0);
    TestTrue(TEXT("partial heal -> health == 70"), FMath::Abs(H.Health - 70.0) < Eps);
    H.Heal(999.0);
    TestTrue(TEXT("over-heal caps at 100"), FMath::Abs(H.Health - 100.0) < Eps);
    return true;
}

// --- test_heal_ignores_negative ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthHealIgnoresNegativeTest,
    "GTC.Player.Health.HealIgnoresNegative",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthHealIgnoresNegativeTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    H.Apply(40.0);
    H.Heal(-10.0);
    TestTrue(TEXT("health == 60 (negative ignored)"), FMath::Abs(H.Health - 60.0) < Eps);
    return true;
}

// --- test_armor_absorbs_before_health ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthArmorAbsorbsTest,
    "GTC.Player.Health.ArmorAbsorbsBeforeHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthArmorAbsorbsTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    H.AddArmor(50.0);
    H.Apply(30.0);
    TestTrue(TEXT("armor == 20"), FMath::Abs(H.Armor - 20.0) < Eps);
    TestTrue(TEXT("health == 100"), FMath::Abs(H.Health - 100.0) < Eps);
    return true;
}

// --- test_damage_overflows_armor_into_health ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthDamageOverflowsArmorTest,
    "GTC.Player.Health.DamageOverflowsArmorIntoHealth",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthDamageOverflowsArmorTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0);
    H.AddArmor(20.0);
    H.Apply(50.0);
    TestTrue(TEXT("armor == 0"), FMath::Abs(H.Armor) < Eps);
    TestTrue(TEXT("health == 70"), FMath::Abs(H.Health - 70.0) < Eps);
    return true;
}

// --- test_add_armor_caps_at_max ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthAddArmorCapsTest,
    "GTC.Player.Health.AddArmorCapsAtMax",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthAddArmorCapsTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0, 10.0, 5.0, 80.0);
    H.AddArmor(200.0);
    TestTrue(TEXT("armor caps at 80"), FMath::Abs(H.Armor - 80.0) < Eps);
    return true;
}

// --- test_armor_fraction ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthArmorFractionTest,
    "GTC.Player.Health.ArmorFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthArmorFractionTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(100.0, 10.0, 5.0, 100.0);
    H.AddArmor(40.0);
    TestTrue(TEXT("armor_fraction == 0.4"), FMath::Abs(H.ArmorFraction() - 0.4) < Eps);
    return true;
}

// --- test_revive_clears_armor ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthReviveClearsArmorTest,
    "GTC.Player.Health.ReviveClearsArmor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthReviveClearsArmorTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(80.0);
    H.AddArmor(50.0);
    H.Apply(80.0);
    H.Revive();
    TestTrue(TEXT("health == 80"), FMath::Abs(H.Health - 80.0) < Eps);
    TestTrue(TEXT("armor == 0"), FMath::Abs(H.Armor) < Eps);
    return true;
}

// --- test_revive_restores_full ---
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthReviveRestoresFullTest,
    "GTC.Player.Health.ReviveRestoresFull",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthReviveRestoresFullTest::RunTest(const FString& Parameters)
{
    FPlayerHealthModel H(80.0);
    H.Apply(80.0);
    H.Revive();
    TestTrue(TEXT("health == 80"), FMath::Abs(H.Health - 80.0) < Eps);
    TestFalse(TEXT("not dead"), H.IsDead());
    return true;
}

// ============================================================================
// COMPONENT-BEHAVIOR tests — ownership / lifecycle / delegate firing.
// These layer thin checks on top of the 18 parity funcs (NOT part of the count).
// ============================================================================

// Lifecycle + delegate firing on a real spawned actor in a transient world.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPlayerHealthComponentLifecycleTest,
    "GTC.Player.Health.ComponentLifecycleAndDelegates",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FPlayerHealthComponentLifecycleTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("transient world created"), World))
    {
        return false;
    }
    FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
    Ctx.SetCurrentWorld(World);

    AActor* Owner = World->SpawnActor<AActor>();
    UPlayerHealthComponent* Comp = NewObject<UPlayerHealthComponent>(Owner);

    // Capture delegate fires via a real UObject listener (dynamic delegates only
    // bind to UFUNCTIONs). Bind BEFORE RegisterComponent so BeginPlay's announce
    // is observed.
    UPlayerHealthTestListener* Listener = NewObject<UPlayerHealthTestListener>(Owner);
    Comp->OnHealthChanged.AddDynamic(Listener, &UPlayerHealthTestListener::HandleHealth);
    Comp->OnArmorChanged.AddDynamic(Listener, &UPlayerHealthTestListener::HandleArmor);
    Comp->OnDied.AddDynamic(Listener, &UPlayerHealthTestListener::HandleDeath);

    Comp->RegisterComponent();

    World->InitializeActorsForPlay(FURL());
    Owner->DispatchBeginPlay();

    // BeginPlay announces the seeded full-health / zero-armor state.
    TestTrue(TEXT("starts at full health"), FMath::Abs(Comp->GetHealth() - 100.0f) < Eps);
    TestTrue(TEXT("health announced on init"), Listener->HealthFires >= 1);
    TestTrue(TEXT("armor announced on init"), Listener->ArmorFires >= 1);

    // W3 ARMOR-OWNERSHIP RESOLUTION (docs/W3_WIRING_NOTES.md): the health
    // component is now HEALTH-ONLY — its model is constructed with ArmorMax=0,
    // so its armor pool is neutralized. AddArmor clamps to 0 (the SOLE armor
    // pool now lives on UPlayerStatsComponent, owned by AGTCPlayerState). So
    // AddArmor is a no-op here and must NOT move the accessor.
    const int32 ArmorFiresBefore = Listener->ArmorFires;
    Comp->AddArmor(40.0f);
    TestTrue(TEXT("armor stays 0 (ArmorMax=0 neutralizes the pool)"), FMath::Abs(Comp->GetArmor()) < Eps);
    TestEqual(TEXT("no armor broadcast (no-op, pool neutralized)"), Listener->ArmorFires, ArmorFiresBefore);

    // AddArmor with a negative is likewise a no-op.
    Comp->AddArmor(-5.0f);
    TestEqual(TEXT("no armor broadcast on negative no-op"), Listener->ArmorFires, ArmorFiresBefore);

    // Damage hits health directly (no armor to soak), fires the health delegate.
    // 50 damage -> health 50 (health-only model, no armor absorption).
    const int32 HealthFiresBefore = Listener->HealthFires;
    const bool bKilled = Comp->ApplyDamage(50.0f);
    TestFalse(TEXT("non-lethal hit"), bKilled);
    TestTrue(TEXT("armor still 0 (no second pool)"), FMath::Abs(Comp->GetArmor()) < Eps);
    TestTrue(TEXT("health 50 (full damage hits health)"), FMath::Abs(Comp->GetHealth() - 50.0f) < Eps);
    TestTrue(TEXT("health delegate fired"), Listener->HealthFires > HealthFiresBefore);
    TestTrue(TEXT("health delegate payload"), FMath::Abs(Listener->LastHealth - 50.0f) < Eps);

    // Heal back up; over-heal caps and the accessor reflects it.
    Comp->Heal(999.0f);
    TestTrue(TEXT("heal caps at max"), FMath::Abs(Comp->GetHealth() - 100.0f) < Eps);

    // Lethal hit broadcasts OnDied exactly once and flips IsDead.
    const int32 DeathFiresBefore = Listener->DeathFires;
    const bool bLethal = Comp->ApplyDamage(500.0f);
    TestTrue(TEXT("lethal hit returns true"), bLethal);
    TestTrue(TEXT("component reports dead"), Comp->IsDead());
    TestEqual(TEXT("death delegate fired once"), Listener->DeathFires, DeathFiresBefore + 1);

    // Revive restores full health, clears armor, re-announces both.
    Comp->Revive();
    TestFalse(TEXT("alive after revive"), Comp->IsDead());
    TestTrue(TEXT("full health after revive"), FMath::Abs(Comp->GetHealth() - 100.0f) < Eps);
    TestTrue(TEXT("armor cleared after revive"), FMath::Abs(Comp->GetArmor()) < Eps);

    // Cleanup.
    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
