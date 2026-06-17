// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../WorldSaveGame.h"
#include "../SaveJson.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// PURE-MODEL PARITY TEST for FWorldSaveGame, the UE port of Godot `save_game.gd`.
// 1:1 with the oracle test_save_game.gd (7 funcs): identical literals/tolerances, compound
// boolean returns SPLIT into independent assertions. The Godot SAMPLE Dictionary is rebuilt
// here over the in-module ordered JSON model so the round-trip semantics match exactly.

namespace
{
    // Oracle SAMPLE:
    //   { "player_pos": [12.5, 1.0, -300.0], "time_of_day": 14.25, "wanted_heat": 3.0,
    //     "missions": { "welcome_to_la": { "done": ["a"], "state": "active" } } }
    TSharedRef<FGtcJsonObject> MakeWorldSaveSample()
    {
        const TSharedRef<FGtcJsonObject> Sample = MakeShared<FGtcJsonObject>();

        TArray<FGtcJsonValuePtr> Pos;
        Pos.Add(FGtcJsonValue::MakeNumber(12.5));
        Pos.Add(FGtcJsonValue::MakeNumber(1.0));
        Pos.Add(FGtcJsonValue::MakeNumber(-300.0));
        Sample->SetArray(TEXT("player_pos"), Pos);

        Sample->SetNumber(TEXT("time_of_day"), 14.25);
        Sample->SetNumber(TEXT("wanted_heat"), 3.0);

        const TSharedRef<FGtcJsonObject> Welcome = MakeShared<FGtcJsonObject>();
        TArray<FGtcJsonValuePtr> Done;
        Done.Add(FGtcJsonValue::MakeString(TEXT("a")));
        Welcome->SetArray(TEXT("done"), Done);
        Welcome->SetString(TEXT("state"), TEXT("active"));

        const TSharedRef<FGtcJsonObject> Missions = MakeShared<FGtcJsonObject>();
        Missions->SetObject(TEXT("welcome_to_la"), Welcome);
        Sample->SetObject(TEXT("missions"), Missions);

        return Sample;
    }

    FString ParityTempPath(const TCHAR* Name)
    {
        return FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("GTCWorldSaveTests"), Name);
    }
}

// Oracle: test_round_trip_preserves_state — back.time_of_day == 14.25 AND back.player_pos[0] == 12.5.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameRoundTripPreservesStateTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.RoundTripPreservesState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameRoundTripPreservesStateTest::RunTest(const FString& Parameters)
{
    const FString Text = FWorldSaveGame::Serialize(MakeWorldSaveSample());
    const TSharedRef<FGtcJsonObject> Back = FWorldSaveGame::Deserialize(Text);

    // Split the oracle's compound assertion.
    TestEqual(TEXT("time_of_day == 14.25"), Back->GetNumber(TEXT("time_of_day")), 14.25, GtcTest::Eps);
    const TArray<FGtcJsonValuePtr> Pos = Back->GetArray(TEXT("player_pos"));
    TestEqual(TEXT("player_pos has 3"), Pos.Num(), 3);
    if (Pos.Num() == 3)
    {
        TestEqual(TEXT("player_pos[0] == 12.5"), Pos[0]->AsNumber(), 12.5, GtcTest::Eps);
    }
    return true;
}

// Oracle: test_nested_data_survives — back.missions.welcome_to_la.done == ["a"].
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameNestedDataSurvivesTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.NestedDataSurvives",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameNestedDataSurvivesTest::RunTest(const FString& Parameters)
{
    const TSharedRef<FGtcJsonObject> Back =
        FWorldSaveGame::Deserialize(FWorldSaveGame::Serialize(MakeWorldSaveSample()));

    const TSharedPtr<FGtcJsonObject> Missions = Back->GetObject(TEXT("missions"));
    TestTrue(TEXT("missions object present"), Missions.IsValid());
    if (Missions.IsValid())
    {
        const TSharedPtr<FGtcJsonObject> Welcome = Missions->GetObject(TEXT("welcome_to_la"));
        TestTrue(TEXT("welcome_to_la present"), Welcome.IsValid());
        if (Welcome.IsValid())
        {
            const TArray<FGtcJsonValuePtr> Done = Welcome->GetArray(TEXT("done"));
            TestEqual(TEXT("done has one entry"), Done.Num(), 1);
            if (Done.Num() == 1)
            {
                TestEqual(TEXT("done == [\"a\"]"), Done[0]->AsString(), FString(TEXT("a")));
            }
        }
    }
    return true;
}

// Oracle: test_bad_json_returns_empty — deserialize("{not valid").is_empty().
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameBadJsonReturnsEmptyTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.BadJsonReturnsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameBadJsonReturnsEmptyTest::RunTest(const FString& Parameters)
{
    const TSharedRef<FGtcJsonObject> Back = FWorldSaveGame::Deserialize(TEXT("{not valid"));
    TestEqual(TEXT("bad json -> empty"), Back->Num(), 0);
    return true;
}

// Oracle: test_wrong_version_rejected — stringify({version:999, state:{x:1}}) -> empty.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameWrongVersionRejectedTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.WrongVersionRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameWrongVersionRejectedTest::RunTest(const FString& Parameters)
{
    // Forge {"version": 999, "state": {"x": 1}} via the same JSON layer.
    const TSharedRef<FGtcJsonObject> Forged = MakeShared<FGtcJsonObject>();
    Forged->SetNumber(TEXT("version"), 999.0);
    const TSharedRef<FGtcJsonObject> Inner = MakeShared<FGtcJsonObject>();
    Inner->SetNumber(TEXT("x"), 1.0);
    Forged->SetObject(TEXT("state"), Inner);

    const TSharedRef<FGtcJsonObject> Back = FWorldSaveGame::Deserialize(GtcJson::Serialize(Forged));
    TestEqual(TEXT("wrong version -> empty"), Back->Num(), 0);
    return true;
}

// Oracle: test_missing_state_rejected — stringify({version:1}) -> empty.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameMissingStateRejectedTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.MissingStateRejected",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameMissingStateRejectedTest::RunTest(const FString& Parameters)
{
    const TSharedRef<FGtcJsonObject> NoState = MakeShared<FGtcJsonObject>();
    NoState->SetNumber(TEXT("version"), 1.0);

    const TSharedRef<FGtcJsonObject> Back = FWorldSaveGame::Deserialize(GtcJson::Serialize(NoState));
    TestEqual(TEXT("missing state -> empty"), Back->Num(), 0);
    return true;
}

// Oracle: test_write_then_read_round_trips — write(SAMPLE,path); read(path); ok AND back.wanted_heat == 3.0.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameWriteThenReadRoundTripsTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.WriteThenReadRoundTrips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameWriteThenReadRoundTripsTest::RunTest(const FString& Parameters)
{
    const FString Path = ParityTempPath(TEXT("test_save_tmp.json"));
    IFileManager::Get().Delete(*Path);

    const bool bOk = FWorldSaveGame::Write(MakeWorldSaveSample(), Path);
    const TSharedRef<FGtcJsonObject> Back = FWorldSaveGame::Read(Path);

    // Split: ok AND wanted_heat == 3.0.
    TestTrue(TEXT("write ok"), bOk);
    TestEqual(TEXT("wanted_heat == 3.0"), Back->GetNumber(TEXT("wanted_heat"), -1.0), 3.0, GtcTest::Eps);

    IFileManager::Get().Delete(*Path);
    return true;
}

// Oracle: test_read_missing_file_is_empty — read("user://does_not_exist_42.json").is_empty().
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FWorldSaveGameReadMissingFileIsEmptyTest,
    "GTC.Systems.SaveWorld.WorldSaveGame.ReadMissingFileIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FWorldSaveGameReadMissingFileIsEmptyTest::RunTest(const FString& Parameters)
{
    const FString Path = ParityTempPath(TEXT("does_not_exist_42.json"));
    IFileManager::Get().Delete(*Path); // ensure absent
    const TSharedRef<FGtcJsonObject> Back = FWorldSaveGame::Read(Path);
    TestEqual(TEXT("missing file -> empty"), Back->Num(), 0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
