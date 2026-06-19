// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SaveSubsystem.h"
#include "../SaveData.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Behavior tests for USaveSubsystem (the SaveManager port). These exercise ownership,
// registration, the Deinitialize lifecycle, and a register->save->load round-trip THROUGH
// a hook. They are NOT a re-test of GtcSaveData's serialization math (that is SaveDataTest).
//
// Driving the subsystem's Initialize() path needs a live FSubsystemCollectionBase, which is
// impractical to spin up headless, so the subsystem is constructed directly and its public
// API driven (registration/save/load/file IO + the Deinitialize hook-clearing contract).
// USaveSubsystem has ClassWithin = UGameInstance, so it MUST be created with a UGameInstance
// outer (a transient one) — a Package outer trips a ClassWithin ensure.

namespace
{
    USaveSubsystem* MakeSubsystem()
    {
        UGameInstance* Outer = NewObject<UGameInstance>(GetTransientPackage());
        return NewObject<USaveSubsystem>(Outer);
    }

    FString TempSavePath(const TCHAR* Name)
    {
        return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GTCSaveTests"), Name);
    }
}

// Registration: hooks register, count/order/has reflect it, re-register replaces in place.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemRegistrationTest,
    "GTC.Systems.Save.SaveSubsystem.Registration",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemRegistrationTest::RunTest(const FString& Parameters)
{
    USaveSubsystem* S = MakeSubsystem();
    TestEqual(TEXT("starts empty"), S->HookCount(), 0);
    TestFalse(TEXT("empty key rejected"),
        S->RegisterHook(TEXT(""), FSaveHook(), FLoadHook()));

    S->RegisterHook(TEXT("player"), FSaveHook(), FLoadHook());
    S->RegisterHook(TEXT("wanted"), FSaveHook(), FLoadHook());
    TestEqual(TEXT("two hooks"), S->HookCount(), 2);
    TestTrue(TEXT("has player"), S->HasHook(TEXT("player")));

    const TArray<FString> Sections = S->Sections();
    TestEqual(TEXT("registration order [0]"), Sections[0], FString(TEXT("player")));
    TestEqual(TEXT("registration order [1]"), Sections[1], FString(TEXT("wanted")));

    // Re-register replaces without growing or reordering.
    S->RegisterHook(TEXT("player"), FSaveHook(), FLoadHook());
    TestEqual(TEXT("re-register no growth"), S->HookCount(), 2);
    TestEqual(TEXT("order kept"), S->Sections()[0], FString(TEXT("player")));
    return true;
}

// Unregister removes the hook and reindexes the remainder.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemUnregisterTest,
    "GTC.Systems.Save.SaveSubsystem.Unregister",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemUnregisterTest::RunTest(const FString& Parameters)
{
    USaveSubsystem* S = MakeSubsystem();
    S->RegisterHook(TEXT("a"), FSaveHook(), FLoadHook());
    S->RegisterHook(TEXT("b"), FSaveHook(), FLoadHook());
    S->RegisterHook(TEXT("c"), FSaveHook(), FLoadHook());

    TestTrue(TEXT("unregister b"), S->UnregisterHook(TEXT("b")));
    TestFalse(TEXT("b gone"), S->HasHook(TEXT("b")));
    TestEqual(TEXT("two remain"), S->HookCount(), 2);
    TestEqual(TEXT("a still first"), S->Sections()[0], FString(TEXT("a")));
    TestEqual(TEXT("c still present"), S->Sections()[1], FString(TEXT("c")));
    TestFalse(TEXT("unregister missing"), S->UnregisterHook(TEXT("zzz")));
    return true;
}

// Deinitialize lifecycle: clears all hooks so none survive the owning GameInstance.
// (Initialize itself needs a live FSubsystemCollectionBase, impractical headless — its
// body only Resets the same containers Deinitialize clears, exercised here.)
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemDeinitializeClearsHooksTest,
    "GTC.Systems.Save.SaveSubsystem.DeinitializeClearsHooks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemDeinitializeClearsHooksTest::RunTest(const FString& Parameters)
{
    USaveSubsystem* S = MakeSubsystem();
    S->RegisterHook(TEXT("player"), FSaveHook(), FLoadHook());
    S->RegisterHook(TEXT("wanted"), FSaveHook(), FLoadHook());
    TestEqual(TEXT("registered"), S->HookCount(), 2);
    S->Deinitialize();
    TestEqual(TEXT("cleared on deinit"), S->HookCount(), 0);
    return true;
}

// Register -> Save -> Load round-trips a section's payload THROUGH the hooks (value-level
// per the tolerance contract; integers/strings exact, the float within Eps).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemHookRoundTripTest,
    "GTC.Systems.Save.SaveSubsystem.HookRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemHookRoundTripTest::RunTest(const FString& Parameters)
{
    USaveSubsystem* S = MakeSubsystem();

    // A coordinator owns this state; its hooks read/write it (non-owning capture).
    double SavedMoney = 1250.0;
    double SavedArmor = 87.5;
    double LoadedMoney = 0.0;
    double LoadedArmor = 0.0;

    FSaveHook OnSave;
    OnSave.BindLambda([&](const TSharedRef<FGtcJsonObject>& Out)
    {
        Out->SetNumber(TEXT("money"), SavedMoney);
        Out->SetNumber(TEXT("armor"), SavedArmor);
    });
    FLoadHook OnLoad;
    OnLoad.BindLambda([&](const TSharedRef<FGtcJsonObject>& In)
    {
        LoadedMoney = In->GetNumber(TEXT("money"));
        LoadedArmor = In->GetNumber(TEXT("armor"));
    });
    S->RegisterHook(TEXT("stats"), OnSave, OnLoad);

    const FString Text = S->BuildSaveText();
    TestEqual(TEXT("text carries version 4"), GtcSaveData::VersionOf(Text), 4);

    TestTrue(TEXT("apply succeeded"), S->ApplySaveText(Text));
    TestEqual(TEXT("money round-trips"), LoadedMoney, SavedMoney, GtcTest::Eps);
    TestEqual(TEXT("armor round-trips"), LoadedArmor, SavedArmor, GtcTest::Eps);
    return true;
}

// Empty/garbage save text restores nothing (corrupt save can't wipe live state). The load
// hook must NOT fire when the decode is empty.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemEmptyDoesNotWipeTest,
    "GTC.Systems.Save.SaveSubsystem.EmptyDoesNotWipe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemEmptyDoesNotWipeTest::RunTest(const FString& Parameters)
{
    USaveSubsystem* S = MakeSubsystem();
    bool LoadFired = false;
    FLoadHook OnLoad;
    OnLoad.BindLambda([&](const TSharedRef<FGtcJsonObject>&) { LoadFired = true; });
    S->RegisterHook(TEXT("stats"), FSaveHook(), OnLoad);

    TestFalse(TEXT("garbage apply returns false"), S->ApplySaveText(TEXT("not json {[")));
    TestFalse(TEXT("load hook never fired"), LoadFired);
    return true;
}

// A registered hook with no saved section gets an empty object ("no saved data for me"),
// never a crash — mirrors Godot snapshot.get(key, {}).
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemMissingSectionIsEmptyTest,
    "GTC.Systems.Save.SaveSubsystem.MissingSectionIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemMissingSectionIsEmptyTest::RunTest(const FString& Parameters)
{
    USaveSubsystem* Writer = MakeSubsystem();
    FSaveHook OnSave;
    OnSave.BindLambda([&](const TSharedRef<FGtcJsonObject>& Out) { Out->SetNumber(TEXT("x"), 1.0); });
    Writer->RegisterHook(TEXT("present"), OnSave, FLoadHook());
    const FString Text = Writer->BuildSaveText();

    // A reader that registered a DIFFERENT section than the save contains.
    USaveSubsystem* Reader = MakeSubsystem();
    int32 SeenFields = -1;
    FLoadHook OnLoad;
    OnLoad.BindLambda([&](const TSharedRef<FGtcJsonObject>& In) { SeenFields = In->Num(); });
    Reader->RegisterHook(TEXT("absent"), FSaveHook(), OnLoad);

    TestTrue(TEXT("apply ok"), Reader->ApplySaveText(Text));
    TestEqual(TEXT("missing section -> empty object"), SeenFields, 0);
    return true;
}

// File IO round-trip under Saved/, with cleanup.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveSubsystemFileRoundTripTest,
    "GTC.Systems.Save.SaveSubsystem.FileRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveSubsystemFileRoundTripTest::RunTest(const FString& Parameters)
{
    const FString Path = TempSavePath(TEXT("file_round_trip.json"));
    IFileManager::Get().Delete(*Path);

    USaveSubsystem* S = MakeSubsystem();
    double Saved = 3.14159;
    double Loaded = 0.0;
    FSaveHook OnSave;
    OnSave.BindLambda([&](const TSharedRef<FGtcJsonObject>& Out) { Out->SetNumber(TEXT("v"), Saved); });
    FLoadHook OnLoad;
    OnLoad.BindLambda([&](const TSharedRef<FGtcJsonObject>& In) { Loaded = In->GetNumber(TEXT("v")); });
    S->RegisterHook(TEXT("sec"), OnSave, OnLoad);

    TestTrue(TEXT("missing file load is no-op false"), !S->LoadFromFile(Path));
    TestTrue(TEXT("save to file"), S->SaveToFile(Path));
    TestTrue(TEXT("file exists"), FPaths::FileExists(Path));
    TestTrue(TEXT("load from file"), S->LoadFromFile(Path));
    TestEqual(TEXT("value round-trips through disk"), Loaded, Saved, GtcTest::Eps);

    // Cleanup.
    IFileManager::Get().Delete(*Path);
    TestFalse(TEXT("cleaned up"), FPaths::FileExists(Path));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
