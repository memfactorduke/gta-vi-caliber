// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCHudWidget.h"
#include "../../../Player/Stats/PlayerStats.h"
#include "../../../Player/Health/PlayerHealthComponent.h"
#include "../../../Tests/GtcTestTolerances.h"

#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GameFramework/Actor.h"

using GtcTest::Eps;

// ============================================================================
// GTC.UI.Hud — thin behavior test for the HUD C++ base's delegate subscription
// + cached-value maintenance. A transient UGTCHudWidget (NewObject, no Slate
// construction) binds to real stats/health components and must update its cached
// fractions / money when the components broadcast. The visual WBP / BindWidget
// bars / live PIE pawn wiring are DEFERRED-TO-EDITOR and intentionally untested
// here. Constants are independent of the format test.
// ============================================================================

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcHudWidgetBindAndUpdateTest,
    "GTC.UI.Hud.BindAndUpdate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcHudWidgetBindAndUpdateTest::RunTest(const FString& Parameters)
{
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    if (!TestNotNull(TEXT("transient world created"), World))
    {
        return false;
    }
    FWorldContext& Ctx = GEngine->CreateNewWorldContext(EWorldType::Game);
    Ctx.SetCurrentWorld(World);

    AActor* Owner = World->SpawnActor<AActor>();
    UPlayerStatsComponent* Stats = NewObject<UPlayerStatsComponent>(Owner);
    UPlayerHealthComponent* Health = NewObject<UPlayerHealthComponent>(Owner);
    Stats->RegisterComponent();
    Health->RegisterComponent();

    World->InitializeActorsForPlay(FURL());
    Owner->DispatchBeginPlay();

    // The HUD base only needs a UObject + the delegate API — NewObject (no Slate
    // CreateWidget) is enough to exercise BindToComponents and the handlers.
    UGTCHudWidget* Hud = NewObject<UGTCHudWidget>(Owner);
    if (!TestNotNull(TEXT("hud widget created"), Hud))
    {
        return false;
    }

    // --- Seed-on-bind: cached values reflect current component state ---------
    Stats->AddArmor(40.0f);          // MaxArmor 100 -> fraction 0.40
    Health->ApplyDamage(25.0f);      // 100 - 25 = 75 / 100 -> fraction 0.75
    Hud->BindToComponents(Stats, Health);

    TestTrue(TEXT("seeded armor fraction 0.40"), FMath::Abs(Hud->ArmorFraction - 0.40f) < Eps);
    TestTrue(TEXT("seeded health fraction 0.75"), FMath::Abs(Hud->HealthFraction - 0.75f) < Eps);
    TestEqual(TEXT("seeded money 1500"), Hud->Money, 1500);
    TestEqual(TEXT("seeded money text"), Hud->MoneyText, FString(TEXT("$1,500")));

    // --- Live updates: component broadcasts drive the cached values ----------
    Stats->AddArmor(20.0f);          // 40 + 20 = 60 / 100 -> 0.60
    TestTrue(TEXT("armor fraction updated to 0.60"), FMath::Abs(Hud->ArmorFraction - 0.60f) < Eps);

    Stats->AddMoney(250);            // 1500 + 250 = 1750
    TestEqual(TEXT("money updated to 1750"), Hud->Money, 1750);
    TestEqual(TEXT("money text regrouped"), Hud->MoneyText, FString(TEXT("$1,750")));

    Health->Heal(15.0f);             // 75 + 15 = 90 / 100 -> 0.90
    TestTrue(TEXT("health fraction updated to 0.90"), FMath::Abs(Hud->HealthFraction - 0.90f) < Eps);

    // --- Unbind: no further updates after teardown ---------------------------
    Hud->UnbindComponents();
    Stats->AddMoney(1000);           // would be 2750 if still bound
    TestEqual(TEXT("money frozen after unbind"), Hud->Money, 1750);

    // Re-bind is idempotent (no double-subscription): one AddMoney -> one update.
    Hud->BindToComponents(Stats, Health);
    TestEqual(TEXT("rebind reseeds money to 2750"), Hud->Money, 2750);
    Stats->AddMoney(50);
    TestEqual(TEXT("single update after rebind"), Hud->Money, 2800);

    Hud->UnbindComponents();

    GEngine->DestroyWorldContext(World);
    World->DestroyWorld(false);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
