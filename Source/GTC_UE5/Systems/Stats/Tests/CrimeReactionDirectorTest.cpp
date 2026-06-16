// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CrimeReactionDirector.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// Subsystem LOGIC-PARITY tests (Wave 2). A Godot oracle DOES exist —
// game/tests/crime_reaction_probe.gd (a SceneTree wiring probe) — which asserts that a
// wanted spike files a news headline AND heats the active district (heat > 0), then that
// _process decay cools it (heat drops). These tests align with that logic via the
// explicit drivers: OnStarsChanged files news + adds heat, Tick decays it. Only the
// scene-graph wiring exercised by the probe (resolving the "wanted" group, binding
// stars_changed) and the radio/news ACTOR wiring are deferred to Wave 3. The exact
// constants here (0.30 heat, 0.43 after decay, headline strings) are stronger than the
// probe's >0 / monotonic-drop checks. FDistrictEconomy is the merged Economy model reused
// via #include. Created with a transient UGameInstance outer.

namespace
{
    UCrimeReactionDirector* MakeCrimeReactionDirectorForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<UCrimeReactionDirector>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeReactionRisingStarsHeatsAndNewsTest,
    "GTC.Systems.CrimeReaction.RisingStarsHeatsDistrictAndFilesNews",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeReactionRisingStarsHeatsAndNewsTest::RunTest(const FString& Parameters)
{
    UCrimeReactionDirector* Director = MakeCrimeReactionDirectorForTest();
    // Gain 2 stars: heat += HeatPerStar * 2 on the active district; a "crime" headline
    // (below RampageStars=4) at severity 2 is filed.
    Director->OnStarsChanged(2);
    TestEqual(TEXT("downtown heat == 0.30"),
        Director->GetDistricts().HeatIn(TEXT("downtown")), 0.30, Eps);
    TestTrue(TEXT("news pending"), Director->GetNews().HasPending());
    TestEqual(TEXT("crime severity-2 headline"),
        Director->GetNews().PeekLatest(), FString(TEXT("Armed robbery rocks downtown.")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeReactionRampageThresholdTest,
    "GTC.Systems.CrimeReaction.HighStarsFileRampageHeadline",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeReactionRampageThresholdTest::RunTest(const FString& Parameters)
{
    UCrimeReactionDirector* Director = MakeCrimeReactionDirectorForTest();
    // 4 stars (>= RampageStars): a "rampage" headline (3 tiers, severity 4 clamps to last).
    Director->OnStarsChanged(4);
    TestEqual(TEXT("rampage headline"),
        Director->GetNews().PeekLatest(), FString(TEXT("downtown locked down amid a violent rampage.")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeReactionNoHeatOnFallTest,
    "GTC.Systems.CrimeReaction.FallingStarsDoNotHeatOrFileNews",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeReactionNoHeatOnFallTest::RunTest(const FString& Parameters)
{
    UCrimeReactionDirector* Director = MakeCrimeReactionDirectorForTest();
    Director->OnStarsChanged(3);
    const double HeatAfterRise = Director->GetDistricts().HeatIn(TEXT("downtown"));
    const int32 PendingAfterRise = Director->GetNews().PendingCount();
    // Stars fall: no extra heat, no new headline (Godot only reacts when stars > last).
    Director->OnStarsChanged(1);
    TestEqual(TEXT("heat unchanged on fall"),
        Director->GetDistricts().HeatIn(TEXT("downtown")), HeatAfterRise, Eps);
    TestEqual(TEXT("no new headline on fall"), Director->GetNews().PendingCount(), PendingAfterRise);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeReactionDecayTest,
    "GTC.Systems.CrimeReaction.TickDecaysHeat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FCrimeReactionDecayTest::RunTest(const FString& Parameters)
{
    UCrimeReactionDirector* Director = MakeCrimeReactionDirectorForTest();
    Director->OnStarsChanged(3);  // heat = 0.15 * 3 = 0.45
    const double Before = Director->GetDistricts().HeatIn(TEXT("downtown"));
    // Tick 1s: bleeds HeatDecayPerSec (0.02) off; 0.45 -> 0.43.
    Director->Tick(1.0);
    const double After = Director->GetDistricts().HeatIn(TEXT("downtown"));
    TestTrue(TEXT("heat decayed"), After < Before);
    TestEqual(TEXT("heat == 0.43"), After, 0.43, Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
