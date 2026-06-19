// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SaveData.h"
#include "../../../Tests/GtcTestTolerances.h"

// Each test below maps 1:1 to a test_* function in the the reference reference behavior
// game/tests/unit/test_save_data.gd (21 funcs). Float compares use GtcTest::Eps per the
// round-trip-tolerance contract documented in SaveData.h: JSON number text is not
// byte-identical across the Godot<->UE boundary, only value-within-tolerance.

namespace
{
    TSharedRef<FGtcJsonObject> MakeObj()
    {
        return MakeShared<FGtcJsonObject>();
    }

    // A JSON array value of doubles.
    FGtcJsonValuePtr NumArray(std::initializer_list<double> Vals)
    {
        TArray<FGtcJsonValuePtr> Arr;
        for (double V : Vals)
        {
            Arr.Add(FGtcJsonValue::MakeNumber(V));
        }
        return FGtcJsonValue::MakeArray(Arr);
    }
}

// 1) test_round_trip_preserves_values
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataRoundTripPreservesValuesTest,
    "GTC.Systems.Save.SaveData.RoundTripPreservesValues",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataRoundTripPreservesValuesTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> Snap = MakeObj();
    Snap->SetNumber(TEXT("health"), 72.5);
    Snap->SetNumber(TEXT("heat"), 4.0);
    Snap->SetString(TEXT("name"), TEXT("player"));
    TSharedRef<FGtcJsonObject> Decoded = GtcSaveData::Decode(GtcSaveData::Encode(Snap));
    TestEqual(TEXT("health == 72.5"), Decoded->GetNumber(TEXT("health")), 72.5, GtcTest::Eps);
    TestEqual(TEXT("heat == 4.0"), Decoded->GetNumber(TEXT("heat")), 4.0, GtcTest::Eps);
    TestEqual(TEXT("name == player"), Decoded->GetString(TEXT("name")), FString(TEXT("player")));
    return true;
}

// 2) test_round_trip_nested_array
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataRoundTripNestedArrayTest,
    "GTC.Systems.Save.SaveData.RoundTripNestedArray",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataRoundTripNestedArrayTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> Snap = MakeObj();
    Snap->Set(TEXT("pos"), NumArray({ 1.0, 2.0, 3.0 }));
    TSharedRef<FGtcJsonObject> Decoded = GtcSaveData::Decode(GtcSaveData::Encode(Snap));
    const TArray<FGtcJsonValuePtr> Pos = Decoded->GetArray(TEXT("pos"));
    TestEqual(TEXT("size == 3"), Pos.Num(), 3);
    TestEqual(TEXT("pos[2] == 3.0"), Pos[2]->AsNumber(), 3.0, GtcTest::Eps);
    return true;
}

// 3) test_encode_embeds_version
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataEncodeEmbedsVersionTest,
    "GTC.Systems.Save.SaveData.EncodeEmbedsVersion",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataEncodeEmbedsVersionTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("version_of(encode({})) == Version"),
        GtcSaveData::VersionOf(GtcSaveData::Encode(MakeObj())), GtcSaveData::Version);
    return true;
}

// 4) test_decode_garbage_is_empty
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataDecodeGarbageIsEmptyTest,
    "GTC.Systems.Save.SaveData.DecodeGarbageIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataDecodeGarbageIsEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("garbage decodes empty"),
        GtcSaveData::Decode(TEXT("not json at all {[")).Get().Num(), 0);
    return true;
}

// 5) test_decode_non_object_is_empty
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataDecodeNonObjectIsEmptyTest,
    "GTC.Systems.Save.SaveData.DecodeNonObjectIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataDecodeNonObjectIsEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("[1,2,3] decodes empty"),
        GtcSaveData::Decode(TEXT("[1, 2, 3]")).Get().Num(), 0);
    return true;
}

// 6) test_decode_missing_data_is_empty
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataDecodeMissingDataIsEmptyTest,
    "GTC.Systems.Save.SaveData.DecodeMissingDataIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataDecodeMissingDataIsEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("{version:1} decodes empty"),
        GtcSaveData::Decode(TEXT("{\"version\": 1}")).Get().Num(), 0);
    return true;
}

// 7) test_decode_non_dict_data_is_empty
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataDecodeNonDictDataIsEmptyTest,
    "GTC.Systems.Save.SaveData.DecodeNonDictDataIsEmpty",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataDecodeNonDictDataIsEmptyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("data:42 decodes empty"),
        GtcSaveData::Decode(TEXT("{\"version\": 1, \"data\": 42}")).Get().Num(), 0);
    return true;
}

// 8) test_version_of_garbage_is_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataVersionOfGarbageIsZeroTest,
    "GTC.Systems.Save.SaveData.VersionOfGarbageIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataVersionOfGarbageIsZeroTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("version_of(garbage) == 0"), GtcSaveData::VersionOf(TEXT("???")), 0);
    return true;
}

// 9) test_empty_snapshot_round_trips
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataEmptySnapshotRoundTripsTest,
    "GTC.Systems.Save.SaveData.EmptySnapshotRoundTrips",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataEmptySnapshotRoundTripsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("empty round-trips empty"),
        GtcSaveData::Decode(GtcSaveData::Encode(MakeObj())).Get().Num(), 0);
    return true;
}

// 10) test_vec3_round_trip
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataVec3RoundTripTest,
    "GTC.Systems.Save.SaveData.Vec3RoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataVec3RoundTripTest::RunTest(const FString& Parameters)
{
    const FVector V(1.5, -2.0, 3.25);
    const FGtcJsonValuePtr Arr = FGtcJsonValue::MakeArray(GtcSaveData::Vec3ToArray(V));
    const FVector Back = GtcSaveData::ArrayToVec3(Arr, FVector::ZeroVector);
    TestTrue(TEXT("vec3 round-trips within Eps"), Back.Equals(V, GtcTest::Eps));
    return true;
}

// 11) test_array_to_vec3_rejects_malformed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataArrayToVec3RejectsMalformedTest,
    "GTC.Systems.Save.SaveData.ArrayToVec3RejectsMalformed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataArrayToVec3RejectsMalformedTest::RunTest(const FString& Parameters)
{
    const FVector Fb(9, 9, 9);
    const FGtcJsonValuePtr NotArray = FGtcJsonValue::MakeString(TEXT("nope"));
    const FGtcJsonValuePtr TooShort = NumArray({ 1.0, 2.0 });
    TArray<FGtcJsonValuePtr> Mixed;
    Mixed.Add(FGtcJsonValue::MakeNumber(1.0));
    Mixed.Add(FGtcJsonValue::MakeNumber(2.0));
    Mixed.Add(FGtcJsonValue::MakeString(TEXT("x")));
    const FGtcJsonValuePtr NonNumeric = FGtcJsonValue::MakeArray(Mixed);

    TestTrue(TEXT("non-array -> fb"), GtcSaveData::ArrayToVec3(NotArray, Fb).Equals(Fb, GtcTest::Eps));
    TestTrue(TEXT("size 2 -> fb"), GtcSaveData::ArrayToVec3(TooShort, Fb).Equals(Fb, GtcTest::Eps));
    TestTrue(TEXT("non-numeric -> fb"), GtcSaveData::ArrayToVec3(NonNumeric, Fb).Equals(Fb, GtcTest::Eps));
    return true;
}

// 12) test_transform_round_trip
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataTransformRoundTripTest,
    "GTC.Systems.Save.SaveData.TransformRoundTrip",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataTransformRoundTripTest::RunTest(const FString& Parameters)
{
    const FRotator Rot(17.0, 40.0, -11.0);
    const FTransform T(Rot, FVector(4, 5, 6));
    const FGtcJsonValuePtr Dict = FGtcJsonValue::MakeObject(GtcSaveData::TransformToDict(T));
    const FTransform Back = GtcSaveData::DictToTransform(Dict, FTransform::Identity);
    TestTrue(TEXT("origin within Eps"), Back.GetLocation().Equals(T.GetLocation(), GtcTest::Eps));
    TestTrue(TEXT("rotation within Eps"),
        Back.GetRotation().Rotator().Equals(T.GetRotation().Rotator(), GtcTest::Eps));
    return true;
}

// 13) test_dict_to_transform_rejects_malformed
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataDictToTransformRejectsMalformedTest,
    "GTC.Systems.Save.SaveData.DictToTransformRejectsMalformed",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataDictToTransformRejectsMalformedTest::RunTest(const FString& Parameters)
{
    const FTransform Fb(FRotator::ZeroRotator, FVector(7, 7, 7));
    const FGtcJsonValuePtr Garbage = FGtcJsonValue::MakeString(TEXT("garbage"));
    const FTransform Back = GtcSaveData::DictToTransform(Garbage, Fb);
    TestTrue(TEXT("malformed -> fb origin"), Back.GetLocation().Equals(Fb.GetLocation(), GtcTest::Eps));
    return true;
}

// 14) test_number_or_falls_back
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataNumberOrFallsBackTest,
    "GTC.Systems.Save.SaveData.NumberOrFallsBack",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataNumberOrFallsBackTest::RunTest(const FString& Parameters)
{
    const FGtcJsonValuePtr Flt = FGtcJsonValue::MakeNumber(42.0);
    const FGtcJsonValuePtr Int = FGtcJsonValue::MakeNumber(3);
    const FGtcJsonValuePtr Str = FGtcJsonValue::MakeString(TEXT("x"));
    const FGtcJsonValuePtr Null = FGtcJsonValue::MakeNull();
    TestEqual(TEXT("42.0 kept"), GtcSaveData::NumberOr(Flt, 0.0), 42.0, GtcTest::Eps);
    TestEqual(TEXT("int 3 -> 3.0"), GtcSaveData::NumberOr(Int, 0.0), 3.0, GtcTest::Eps);
    TestEqual(TEXT("string -> fb"), GtcSaveData::NumberOr(Str, -1.0), -1.0, GtcTest::Eps);
    TestEqual(TEXT("null -> fb"), GtcSaveData::NumberOr(Null, -1.0), -1.0, GtcTest::Eps);
    return true;
}

// 15) test_version_is_four
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataVersionIsFourTest,
    "GTC.Systems.Save.SaveData.VersionIsFour",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataVersionIsFourTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("Version == 4"), GtcSaveData::Version, 4);
    return true;
}

// 16) test_migrate_v1_fills_new_sections
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataMigrateV1FillsNewSectionsTest,
    "GTC.Systems.Save.SaveData.MigrateV1FillsNewSections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataMigrateV1FillsNewSectionsTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> V1 = MakeObj();
    V1->Set(TEXT("player_pos"), NumArray({ 1.0, 2.0, 3.0 }));
    TSharedRef<FGtcJsonObject> Health = MakeObj();
    Health->SetNumber(TEXT("hp"), 50.0);
    V1->SetObject(TEXT("health"), Health);

    TSharedRef<FGtcJsonObject> M = GtcSaveData::Migrate(V1, 1);
    TestTrue(TEXT("stats is object"), M->HasObjectField(TEXT("stats")));
    TestEqual(TEXT("stats empty"), M->GetObject(TEXT("stats"))->Num(), 0);
    TestTrue(TEXT("progression is object"), M->HasObjectField(TEXT("progression")));
    TestTrue(TEXT("properties is object"), M->HasObjectField(TEXT("properties")));
    TestTrue(TEXT("lifetime_stats is object"), M->HasObjectField(TEXT("lifetime_stats")));
    TestTrue(TEXT("player_skills is object"), M->HasObjectField(TEXT("player_skills")));
    TestTrue(TEXT("player_pos preserved"), M->Has(TEXT("player_pos")));
    TestEqual(TEXT("health.hp preserved"),
        M->GetObject(TEXT("health"))->GetNumber(TEXT("hp")), 50.0, GtcTest::Eps);
    return true;
}

// 17) test_migrate_v2_fills_lifetime_stats
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataMigrateV2FillsLifetimeStatsTest,
    "GTC.Systems.Save.SaveData.MigrateV2FillsLifetimeStats",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataMigrateV2FillsLifetimeStatsTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> V2 = MakeObj();
    TSharedRef<FGtcJsonObject> Stats = MakeObj();
    Stats->SetNumber(TEXT("money"), 900);
    V2->SetObject(TEXT("stats"), Stats);
    TSharedRef<FGtcJsonObject> Prog = MakeObj();
    Prog->SetNumber(TEXT("total_xp"), 12);
    V2->SetObject(TEXT("progression"), Prog);

    TSharedRef<FGtcJsonObject> M = GtcSaveData::Migrate(V2, 2);
    TestTrue(TEXT("lifetime_stats is object"), M->HasObjectField(TEXT("lifetime_stats")));
    TestTrue(TEXT("player_skills is object"), M->HasObjectField(TEXT("player_skills")));
    TestEqual(TEXT("lifetime_stats empty"), M->GetObject(TEXT("lifetime_stats"))->Num(), 0);
    TestEqual(TEXT("stats.money preserved"),
        M->GetObject(TEXT("stats"))->GetNumber(TEXT("money")), 900.0, GtcTest::Eps);
    return true;
}

// 18) test_migrate_v3_fills_player_skills
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataMigrateV3FillsPlayerSkillsTest,
    "GTC.Systems.Save.SaveData.MigrateV3FillsPlayerSkills",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataMigrateV3FillsPlayerSkillsTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> V3 = MakeObj();
    TSharedRef<FGtcJsonObject> Lifetime = MakeObj();
    TSharedRef<FGtcJsonObject> Inner = MakeObj();
    Inner->SetNumber(TEXT("missions_passed"), 2.0);
    Lifetime->SetObject(TEXT("stats"), Inner);
    V3->SetObject(TEXT("lifetime_stats"), Lifetime);

    TSharedRef<FGtcJsonObject> M = GtcSaveData::Migrate(V3, 3);
    TestTrue(TEXT("player_skills is object"), M->HasObjectField(TEXT("player_skills")));
    TestEqual(TEXT("player_skills empty"), M->GetObject(TEXT("player_skills"))->Num(), 0);
    TestEqual(TEXT("lifetime_stats.stats.missions_passed preserved"),
        M->GetObject(TEXT("lifetime_stats"))->GetObject(TEXT("stats"))
            ->GetNumber(TEXT("missions_passed")), 2.0, GtcTest::Eps);
    return true;
}

// 19) test_migrate_current_version_is_untouched
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataMigrateCurrentVersionUntouchedTest,
    "GTC.Systems.Save.SaveData.MigrateCurrentVersionUntouched",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataMigrateCurrentVersionUntouchedTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> V = MakeObj();
    TSharedRef<FGtcJsonObject> Stats = MakeObj();
    Stats->SetNumber(TEXT("money"), 900);
    V->SetObject(TEXT("stats"), Stats);
    V->SetObject(TEXT("vehicles"), MakeObj());

    TSharedRef<FGtcJsonObject> M = GtcSaveData::Migrate(V, GtcSaveData::Version);
    // Structural equality: same key set, no new sections added.
    TestEqual(TEXT("same key count"), M->Num(), V->Num());
    TestTrue(TEXT("has stats"), M->Has(TEXT("stats")));
    TestTrue(TEXT("has vehicles"), M->Has(TEXT("vehicles")));
    TestFalse(TEXT("no lifetime_stats added"), M->Has(TEXT("lifetime_stats")));
    TestFalse(TEXT("no player_skills added"), M->Has(TEXT("player_skills")));
    TestEqual(TEXT("stats.money preserved"),
        M->GetObject(TEXT("stats"))->GetNumber(TEXT("money")), 900.0, GtcTest::Eps);
    return true;
}

// 20) test_migrate_does_not_mutate_input
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataMigrateDoesNotMutateInputTest,
    "GTC.Systems.Save.SaveData.MigrateDoesNotMutateInput",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataMigrateDoesNotMutateInputTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> V1 = MakeObj();
    TSharedRef<FGtcJsonObject> Health = MakeObj();
    Health->SetNumber(TEXT("hp"), 10.0);
    V1->SetObject(TEXT("health"), Health);

    GtcSaveData::Migrate(V1, 1);
    TestFalse(TEXT("input not mutated (no stats key)"), V1->Has(TEXT("stats")));
    return true;
}

// 21) test_migrate_preserves_existing_sections_from_v1
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSaveDataMigratePreservesExistingSectionsTest,
    "GTC.Systems.Save.SaveData.MigratePreservesExistingSections",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSaveDataMigratePreservesExistingSectionsTest::RunTest(const FString& Parameters)
{
    TSharedRef<FGtcJsonObject> Odd = MakeObj();
    TSharedRef<FGtcJsonObject> Stats = MakeObj();
    Stats->SetNumber(TEXT("money"), 123);
    Odd->SetObject(TEXT("stats"), Stats);

    TSharedRef<FGtcJsonObject> M = GtcSaveData::Migrate(Odd, 1);
    TestEqual(TEXT("existing stats.money kept verbatim"),
        static_cast<int32>(M->GetObject(TEXT("stats"))->GetNumber(TEXT("money"))), 123);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
