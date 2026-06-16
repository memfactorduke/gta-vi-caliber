// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CharacterRosterSubsystem.h"
#include "RosterTestListener.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Engine/GameInstance.h"

using GtcTest::Eps;

// Subsystem BEHAVIOR tests (Wave 2 rule): these verify UCharacterRosterSubsystem OWNS its
// FCharacterRoster and that its drivers (Initialize / RequestSwitch / money accessors)
// wire it correctly. The Godot character_switcher.gd has NO parity oracle, so these are
// behavior assertions for the port, NOT 1:1 parity (the roster math itself is covered 1:1
// by the 14 CharacterRoster parity tests). The live PlayerStats/Wanted wallet sync is
// Wave-3 and DEFERRED (see CharacterRosterSubsystem.h).

namespace
{
    // UGameInstanceSubsystem requires a UGameInstance outer; a transient one suffices.
    // Driving Initialize() needs a live FSubsystemCollectionBase, impractical to spin up
    // headless (same constraint WantedSubsystemTest / SaveSubsystemTest document), so the
    // subsystem is constructed directly and Initialize seeds the roster identically to the
    // collection-driven path. System-prefixed helper name (ODR: unique vs main + own files).
    UCharacterRosterSubsystem* MakeRosterSubsystemForTest()
    {
        UGameInstance* GameInstance = NewObject<UGameInstance>(GetTransientPackage());
        // The owned FCharacterRoster member is default-constructed (DefaultCharacters, first
        // lead active), so the construction-only path is equivalent to the collection-driven
        // Initialize for these defaults — Deinitialize is still driven to exercise lifecycle.
        return NewObject<UCharacterRosterSubsystem>(GameInstance);
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSubsystemInitDefaultsTest,
    "GTC.Systems.Roster.CharacterRosterSubsystem.InitializesDefaultRoster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSubsystemInitDefaultsTest::RunTest(const FString& Parameters)
{
    UCharacterRosterSubsystem* Subsystem = MakeRosterSubsystemForTest();
    // Owns a default dual-protagonist roster with the first lead active (matches Godot
    // CharacterSwitcher._init -> CharacterRoster.new()). The owned roster's own ctor seeds
    // this even without the collection-driven Initialize.
    TestEqual(TEXT("count == 2"), Subsystem->CharacterCount(), 2);
    TestEqual(TEXT("active == mara"), Subsystem->ActiveId(), FString(TEXT("mara")));
    TestEqual(TEXT("active_name == Mara"), Subsystem->ActiveName(), FString(TEXT("Mara")));
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSubsystemRequestSwitchDrivesRosterTest,
    "GTC.Systems.Roster.CharacterRosterSubsystem.RequestSwitchDrivesOwnedRoster",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSubsystemRequestSwitchDrivesRosterTest::RunTest(const FString& Parameters)
{
    UCharacterRosterSubsystem* Subsystem = MakeRosterSubsystemForTest();
    const bool bOk = Subsystem->RequestSwitch(TEXT("rico"));
    TestTrue(TEXT("switch accepted"), bOk);
    TestEqual(TEXT("active flipped to rico"), Subsystem->ActiveId(), FString(TEXT("rico")));
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSubsystemRequestSwitchRejectsTest,
    "GTC.Systems.Roster.CharacterRosterSubsystem.RequestSwitchRejectedDoesNotFlip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSubsystemRequestSwitchRejectsTest::RunTest(const FString& Parameters)
{
    UCharacterRosterSubsystem* Subsystem = MakeRosterSubsystemForTest();
    // Unknown id and already-active are both refused by the owned roster; active stays put.
    TestFalse(TEXT("unknown refused"), Subsystem->RequestSwitch(TEXT("ghost")));
    TestFalse(TEXT("active refused"), Subsystem->RequestSwitch(TEXT("mara")));
    TestEqual(TEXT("active unchanged"), Subsystem->ActiveId(), FString(TEXT("mara")));
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSubsystemSwitchBroadcastsTest,
    "GTC.Systems.Roster.CharacterRosterSubsystem.SwitchBroadcastsOnCharacterSwitched",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSubsystemSwitchBroadcastsTest::RunTest(const FString& Parameters)
{
    UCharacterRosterSubsystem* Subsystem = MakeRosterSubsystemForTest();
    // The Godot character_switched(id) signal is mirrored as OnCharacterSwitched (a dynamic
    // multicast delegate); a UFUNCTION listener captures the broadcast.
    URosterTestSwitchListener* Listener = NewObject<URosterTestSwitchListener>(GetTransientPackage());
    Subsystem->OnCharacterSwitched.AddDynamic(Listener, &URosterTestSwitchListener::OnSwitched);
    // A refused switch must NOT broadcast; an accepted one broadcasts the new id exactly once.
    Subsystem->RequestSwitch(TEXT("mara")); // already active -> refused
    TestEqual(TEXT("no broadcast on refusal"), Listener->HeardCount, 0);
    Subsystem->RequestSwitch(TEXT("rico"));
    TestEqual(TEXT("broadcast once"), Listener->HeardCount, 1);
    TestEqual(TEXT("heard rico"), Listener->LastId, FString(TEXT("rico")));
    Subsystem->Deinitialize();
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FRosterSubsystemIndependentWalletsTest,
    "GTC.Systems.Roster.CharacterRosterSubsystem.WalletsStayIndependentAcrossSwitch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRosterSubsystemIndependentWalletsTest::RunTest(const FString& Parameters)
{
    UCharacterRosterSubsystem* Subsystem = MakeRosterSubsystemForTest();
    // Behavior: the subsystem-owned roster keeps each lead's wallet independent across a
    // switch (the dual-protagonist switch-and-resume contract; live PlayerStats sync is W3).
    Subsystem->AddMoney(TEXT("mara"), 1000); // 3500
    Subsystem->RequestSwitch(TEXT("rico"));
    Subsystem->AddMoney(TEXT("rico"), 500); // 2000
    TestEqual(TEXT("mara wallet preserved"), Subsystem->MoneyOf(TEXT("mara")), 3500);
    TestEqual(TEXT("rico wallet preserved"), Subsystem->MoneyOf(TEXT("rico")), 2000);
    Subsystem->Deinitialize();
    return true;
}

#endif // WITH_AUTOMATION_TESTS
