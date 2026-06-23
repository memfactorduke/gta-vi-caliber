// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WantedSubsystem.h"
#include "../../Save/SaveSubsystem.h"
#include "../../../Tests/GtcTestTolerances.h"

#include "Engine/GameInstance.h"

// The wanted level (heat) persists across save/load: UWantedSubsystem registers a "wanted"
// section hook with USaveSubsystem, OnSave writes heat / OnLoad restores it. Driven through
// the framework's BuildSaveText -> ApplySaveText round-trip (no file IO). Both subsystems are
// ClassWithin UGameInstance, so they must share a transient UGameInstance outer.

namespace
{
    using GtcTest::Eps;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWantedSaveRoundTripTest,
    "GTC.Systems.Wanted.SaveRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWantedSaveRoundTripTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI = NewObject<UGameInstance>(GetTransientPackage());
    USaveSubsystem* Save = NewObject<USaveSubsystem>(GI);
    UWantedSubsystem* Wanted = NewObject<UWantedSubsystem>(GI);

    TestTrue(TEXT("hook registers"), Wanted->RegisterSaveHooks(Save));
    // A null save is rejected (idempotent guard).
    TestFalse(TEXT("null save rejected"), Wanted->RegisterSaveHooks(nullptr));

    // Raise the heat, snapshot it.
    Wanted->RestoreHeat(7.5);
    const double SavedHeat = Wanted->SerializeHeat();
    TestTrue(TEXT("heat set above zero"), SavedHeat > 1.0);

    const FString Text = Save->BuildSaveText();

    // Wipe live heat (as a fresh session would start), then load it back.
    Wanted->Clear();
    TestEqual(TEXT("heat cleared before load"), Wanted->SerializeHeat(), 0.0, Eps);

    TestTrue(TEXT("apply succeeded"), Save->ApplySaveText(Text));
    TestEqual(TEXT("heat round-trips"), Wanted->SerializeHeat(), SavedHeat, Eps);

    // Unregister drops the hook so a stale subsystem can't be saved into.
    Wanted->UnregisterSaveHooks();
    return true;
}

#endif // WITH_AUTOMATION_TESTS
