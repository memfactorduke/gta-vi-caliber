// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WorldSerializerSubsystem.h"
#include "../SaveSubsystem.h"
#include "../SaveData.h"
#include "../SaveJson.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

// BEHAVIOR tests for UWorldSerializerSubsystem (the Wave 2 manager). These exercise that it
// REGISTERS its world hook with the merged USaveSubsystem, that a save->load round-trip flows
// through that hook (value-within-tolerance), and the unregister-on-Deinitialize lifecycle.
// They are NOT a re-test of FWorldSaveGame's serialization math (that is WorldSaveGameTest).
//
// Both subsystems are GameInstanceSubsystems with ClassWithin = UGameInstance, so they MUST
// be created under a transient UGameInstance outer; we drive their public API directly and use
// RegisterWith() to wire them headless (the live FSubsystemCollectionBase path is impractical).

namespace
{
    UGameInstance* MakeGameInstance()
    {
        return NewObject<UGameInstance>(GetTransientPackage());
    }

    USaveSubsystem* MakeSave(UGameInstance* Outer)
    {
        return NewObject<USaveSubsystem>(Outer);
    }

    UWorldSerializerSubsystem* MakeWorld(UGameInstance* Outer)
    {
        return NewObject<UWorldSerializerSubsystem>(Outer);
    }
}

// Registration: RegisterWith adds exactly the "world" section to USaveSubsystem; null is rejected.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSerializerRegistrationTest,
    "GTC.Systems.SaveWorld.WorldSerializer.Registration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSerializerRegistrationTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI = MakeGameInstance();
    USaveSubsystem* Save = MakeSave(GI);
    UWorldSerializerSubsystem* World = MakeWorld(GI);

    TestFalse(TEXT("null save rejected"), World->RegisterWith(nullptr));
    TestFalse(TEXT("not registered yet"), World->IsRegistered());

    TestTrue(TEXT("registers with save"), World->RegisterWith(Save));
    TestTrue(TEXT("flag set"), World->IsRegistered());
    TestEqual(TEXT("one section on save"), Save->HookCount(), 1);
    TestTrue(TEXT("world section present"),
        Save->HasHook(UWorldSerializerSubsystem::WorldSection));
    return true;
}

// Round-trip THROUGH the merged hook: live world state -> BuildSaveText -> ApplySaveText
// restores it (integers/strings exact, float within Eps). Uses a SECOND subsystem as the
// loader so we prove the section truly carried the data, not in-place identity.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSerializerHookRoundTripTest,
    "GTC.Systems.SaveWorld.WorldSerializer.HookRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSerializerHookRoundTripTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI = MakeGameInstance();

    // Writer side: populate live world state and register, then build the save text.
    USaveSubsystem* SaveW = MakeSave(GI);
    UWorldSerializerSubsystem* Writer = MakeWorld(GI);
    Writer->RegisterWith(SaveW);

    const TSharedRef<FGtcJsonObject> State = Writer->GetWorldState();
    State->SetNumber(TEXT("time_of_day"), 14.25);
    State->SetString(TEXT("act"), TEXT("intro"));
    const TSharedRef<FGtcJsonObject> Missions = MakeShared<FGtcJsonObject>();
    Missions->SetNumber(TEXT("welcome"), 1.0);
    State->SetObject(TEXT("missions"), Missions);

    const FString Text = SaveW->BuildSaveText();
    TestEqual(TEXT("save text version 4"), GtcSaveData::VersionOf(Text), 4);

    // Reader side: a fresh subsystem with empty state applies the same text through its hook.
    USaveSubsystem* SaveR = MakeSave(GI);
    UWorldSerializerSubsystem* Reader = MakeWorld(GI);
    Reader->RegisterWith(SaveR);
    TestEqual(TEXT("reader starts empty"), Reader->GetWorldState()->Num(), 0);

    TestTrue(TEXT("apply ok"), SaveR->ApplySaveText(Text));

    const TSharedRef<FGtcJsonObject> Loaded = Reader->GetWorldState();
    TestEqual(TEXT("time_of_day round-trips"),
        Loaded->GetNumber(TEXT("time_of_day")), 14.25, GtcTest::Eps);
    TestEqual(TEXT("act round-trips"), Loaded->GetString(TEXT("act")), FString(TEXT("intro")));
    const TSharedPtr<FGtcJsonObject> LoadedMissions = Loaded->GetObject(TEXT("missions"));
    TestTrue(TEXT("nested missions survived"), LoadedMissions.IsValid());
    if (LoadedMissions.IsValid())
    {
        TestEqual(TEXT("welcome flag"), LoadedMissions->GetNumber(TEXT("welcome")), 1.0, GtcTest::Eps);
    }

    // Key order observable: the saved section preserves insertion order.
    const TArray<FString>& Keys = Loaded->OrderedKeys();
    TestEqual(TEXT("ordered key [0]"), Keys.IsValidIndex(0) ? Keys[0] : FString(), FString(TEXT("time_of_day")));
    return true;
}

// Empty/garbage save must not wipe live world state (corrupt save guard from USaveSubsystem).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSerializerEmptyDoesNotWipeTest,
    "GTC.Systems.SaveWorld.WorldSerializer.EmptyDoesNotWipe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSerializerEmptyDoesNotWipeTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI = MakeGameInstance();
    USaveSubsystem* Save = MakeSave(GI);
    UWorldSerializerSubsystem* World = MakeWorld(GI);
    World->RegisterWith(Save);

    World->GetWorldState()->SetNumber(TEXT("live"), 42.0);

    TestFalse(TEXT("garbage apply returns false"), Save->ApplySaveText(TEXT("not json {[")));
    // Load hook must not have fired, so live state is intact.
    TestEqual(TEXT("live state intact"), World->GetWorldState()->GetNumber(TEXT("live"), -1.0), 42.0, GtcTest::Eps);
    return true;
}

// Lifecycle: UnregisterFromSave (and Deinitialize) drop our hook so it can't fire post-death.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSerializerUnregisterLifecycleTest,
    "GTC.Systems.SaveWorld.WorldSerializer.UnregisterLifecycle",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSerializerUnregisterLifecycleTest::RunTest(const FString& Parameters)
{
    UGameInstance* GI = MakeGameInstance();
    USaveSubsystem* Save = MakeSave(GI);
    UWorldSerializerSubsystem* World = MakeWorld(GI);

    World->RegisterWith(Save);
    TestEqual(TEXT("registered on save"), Save->HookCount(), 1);

    World->UnregisterFromSave();
    TestFalse(TEXT("flag cleared"), World->IsRegistered());
    TestEqual(TEXT("hook gone from save"), Save->HookCount(), 0);

    // Deinitialize on an already-unregistered subsystem is safe and idempotent.
    World->Deinitialize();
    TestEqual(TEXT("still zero after deinit"), Save->HookCount(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
