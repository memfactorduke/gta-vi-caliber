// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../SoundPropagation.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to an assertion in the Godot parity oracle
// game/tests/unit/test_sound_propagation.gd.

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationBaseLoudnessOrderTest,
    "GTC.Systems.SoundPropagation.BaseLoudnessExplosionLoudestSilencedQuiet",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationBaseLoudnessOrderTest::RunTest(const FString& Parameters)
{
    const double E = FSoundPropagation::BaseLoudness(ESoundKind::Explosion);
    const double G = FSoundPropagation::BaseLoudness(ESoundKind::Gunshot);
    const double S = FSoundPropagation::BaseLoudness(ESoundKind::SilencedShot);
    TestTrue(TEXT("explosion > gunshot"), E > G);
    TestTrue(TEXT("gunshot > silenced"), G > S);
    TestTrue(TEXT("explosion <= 1.0"), E <= 1.0);
    TestTrue(TEXT("silenced >= 0.0"), S >= 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationUnknownKindZeroTest,
    "GTC.Systems.SoundPropagation.BaseLoudnessUnknownKindIsZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationUnknownKindZeroTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("base_loudness(999) == 0.0"), FSoundPropagation::BaseLoudnessForInt(999), 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationIsAlarmingTest,
    "GTC.Systems.SoundPropagation.IsAlarmingGunshotTrueEngineFalse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationIsAlarmingTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("gunshot alarming"), FSoundPropagation::IsAlarming(ESoundKind::Gunshot));
    TestTrue(TEXT("alarm alarming"), FSoundPropagation::IsAlarming(ESoundKind::Alarm));
    TestFalse(TEXT("engine not alarming"), FSoundPropagation::IsAlarming(ESoundKind::Engine));
    TestFalse(TEXT("footstep not alarming"), FSoundPropagation::IsAlarming(ESoundKind::Footstep));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationSilencedNotAlarmingTest,
    "GTC.Systems.SoundPropagation.SilencedShotIsNotAlarming",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationSilencedNotAlarmingTest::RunTest(const FString& Parameters)
{
    TestFalse(TEXT("silenced not alarming"), FSoundPropagation::IsAlarming(ESoundKind::SilencedShot));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationFallsWithDistanceTest,
    "GTC.Systems.SoundPropagation.PerceivedIntensityFallsWithDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationFallsWithDistanceTest::RunTest(const FString& Parameters)
{
    const double Loud = FSoundPropagation::BaseLoudness(ESoundKind::Gunshot);
    const FVector Src = FVector::ZeroVector;
    const double Near = FSoundPropagation::PerceivedIntensity(Src, FVector(5, 0, 0), Loud, 0.0);
    const double Far = FSoundPropagation::PerceivedIntensity(Src, FVector(50, 0, 0), Loud, 0.0);
    const double AtSource = FSoundPropagation::PerceivedIntensity(Src, Src, Loud, 0.0);
    TestTrue(TEXT("near > far"), Near > Far);
    TestTrue(TEXT("near < at_source"), Near < AtSource);
    TestTrue(TEXT("far > 0"), Far > 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationXZPlaneTest,
    "GTC.Systems.SoundPropagation.PerceivedIntensityUsesXZPlane",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationXZPlaneTest::RunTest(const FString& Parameters)
{
    const double Loud = FSoundPropagation::BaseLoudness(ESoundKind::Alarm);
    const FVector Src = FVector::ZeroVector;
    // Equal XZ distance (3-4-5) but very different height read equal.
    const double Ground = FSoundPropagation::PerceivedIntensity(Src, FVector(3, 0, 4), Loud, 0.0);
    const double Elevated = FSoundPropagation::PerceivedIntensity(Src, FVector(3, 100, 4), Loud, 0.0);
    TestEqual(TEXT("ground == elevated"), Ground, Elevated, Eps);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationAmbientMasksTest,
    "GTC.Systems.SoundPropagation.AmbientMasksQuietSounds",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationAmbientMasksTest::RunTest(const FString& Parameters)
{
    const double Loud = FSoundPropagation::BaseLoudness(ESoundKind::Footstep);
    const FVector Src = FVector::ZeroVector;
    const FVector Listener(8, 0, 0);
    const double Quiet = FSoundPropagation::PerceivedIntensity(Src, Listener, Loud, 0.02);
    const double Noisy = FSoundPropagation::PerceivedIntensity(Src, Listener, Loud, 0.5);
    TestTrue(TEXT("quiet > 0"), Quiet > 0.0);
    TestEqual(TEXT("noisy == 0"), Noisy, 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationClamped01Test,
    "GTC.Systems.SoundPropagation.PerceivedIntensityClamped01",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationClamped01Test::RunTest(const FString& Parameters)
{
    const FVector Src = FVector::ZeroVector;
    const double AtSource = FSoundPropagation::PerceivedIntensity(Src, Src, 1.0, 0.0);
    const double Masked = FSoundPropagation::PerceivedIntensity(Src, FVector(5, 0, 0), 0.3, 1.0);
    TestEqual(TEXT("at_source == 1.0"), AtSource, 1.0);
    TestEqual(TEXT("masked == 0.0"), Masked, 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationAudibleGateTest,
    "GTC.Systems.SoundPropagation.IsAudibleThresholdGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationAudibleGateTest::RunTest(const FString& Parameters)
{
    const FVector Src = FVector::ZeroVector;
    const bool Close = FSoundPropagation::IsAudible(Src, FVector(40, 0, 0), 0.5, 0.0);
    const bool Distant = FSoundPropagation::IsAudible(Src, FVector(80, 0, 0), 0.5, 0.0);
    TestTrue(TEXT("close audible"), Close);
    TestFalse(TEXT("distant inaudible"), Distant);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationAudibleRadiusTest,
    "GTC.Systems.SoundPropagation.AudibleRadiusGrowsWithLoudnessShrinksWithFloor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationAudibleRadiusTest::RunTest(const FString& Parameters)
{
    const double LoudR = FSoundPropagation::AudibleRadius(0.9, 0.05);
    const double QuietR = FSoundPropagation::AudibleRadius(0.3, 0.05);
    const double HighFloorR = FSoundPropagation::AudibleRadius(0.9, 0.2);
    const double ZeroLoud = FSoundPropagation::AudibleRadius(0.0, 0.05);
    const double ZeroFloor = FSoundPropagation::AudibleRadius(0.9, 0.0);
    TestTrue(TEXT("loud_r > quiet_r"), LoudR > QuietR);
    TestTrue(TEXT("high_floor_r < loud_r"), HighFloorR < LoudR);
    TestEqual(TEXT("zero_loud == 0"), ZeroLoud, 0.0);
    TestEqual(TEXT("zero_floor == 0"), ZeroFloor, 0.0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationReactionAlarmingSoonerTest,
    "GTC.Systems.SoundPropagation.ReactionAlarmingReachesAlarmedSooner",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationReactionAlarmingSoonerTest::RunTest(const FString& Parameters)
{
    const double Mid = 0.4;
    const ESoundReaction Alarmed = FSoundPropagation::ReactionFor(Mid, true);
    const ESoundReaction Noticed = FSoundPropagation::ReactionFor(Mid, false);
    TestEqual(TEXT("alarming -> Alarmed"), static_cast<int32>(Alarmed), static_cast<int32>(ESoundReaction::Alarmed));
    TestEqual(TEXT("ambient -> Noticed"), static_cast<int32>(Noticed), static_cast<int32>(ESoundReaction::Noticed));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationReactionUnheardBelowFloorTest,
    "GTC.Systems.SoundPropagation.ReactionUnheardBelowFloor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationReactionUnheardBelowFloorTest::RunTest(const FString& Parameters)
{
    TestEqual(
        TEXT("0.0 -> Unheard"),
        static_cast<int32>(FSoundPropagation::ReactionFor(0.0, true)),
        static_cast<int32>(ESoundReaction::Unheard));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationEmitImmutableTest,
    "GTC.Systems.SoundPropagation.EmitFansOutAndIsImmutable",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationEmitImmutableTest::RunTest(const FString& Parameters)
{
    FSoundPropagation::FListener Near(FVector(3, 0, 0));
    FSoundPropagation::FListener Far(FVector(300, 0, 0));
    TArray<FSoundPropagation::FListener> Listeners{Near, Far};
    const TArray<FSoundPropagation::FListenerResult> Out =
        FSoundPropagation::Emit(FVector::ZeroVector, ESoundKind::Gunshot, Listeners, 0.05);

    TestEqual(TEXT("two results"), Out.Num(), 2);
    TestEqual(
        TEXT("near reaction Alarmed"),
        static_cast<int32>(Out[0].Reaction),
        static_cast<int32>(ESoundReaction::Alarmed));
    TestTrue(TEXT("near heard"), Out[0].bHeard);
    TestFalse(TEXT("far not heard"), Out[1].bHeard);
    // Inputs (and the source Near/Far) are never mutated: Emit returns new structs.
    TestEqual(TEXT("input near pos untouched"), Listeners[0].Pos, FVector(3, 0, 0));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationEmitCarriesKeysTest,
    "GTC.Systems.SoundPropagation.EmitCarriesListenerKeysThrough",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationEmitCarriesKeysTest::RunTest(const FString& Parameters)
{
    TArray<FSoundPropagation::FListener> Listeners{
        FSoundPropagation::FListener(FVector(2, 0, 0), TEXT("cop_3"))};
    const TArray<FSoundPropagation::FListenerResult> Out =
        FSoundPropagation::Emit(FVector::ZeroVector, ESoundKind::Alarm, Listeners, 0.0);
    TestEqual(TEXT("id carried through"), Out[0].Id, FString(TEXT("cop_3")));
    TestTrue(TEXT("has intensity (heard)"), Out[0].bHeard);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationLoudestHeardTest,
    "GTC.Systems.SoundPropagation.LoudestHeardPicksHighestIntensity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationLoudestHeardTest::RunTest(const FString& Parameters)
{
    const FVector Listener = FVector::ZeroVector;
    TArray<FSoundPropagation::FSoundEvent> Events{
        FSoundPropagation::FSoundEvent(FVector(60, 0, 0), ESoundKind::Explosion),
        FSoundPropagation::FSoundEvent(FVector(4, 0, 0), ESoundKind::Footstep),
    };
    // Near footstep out-shouts a far explosion for this listener.
    const FSoundPropagation::FHeardEvent Best =
        FSoundPropagation::LoudestHeard(Listener, Events, 0.0);
    const FSoundPropagation::FHeardEvent None =
        FSoundPropagation::LoudestHeard(FVector(5000, 0, 0), Events, 0.0);
    TestTrue(TEXT("best heard"), Best.bHeard);
    TestEqual(
        TEXT("best kind footstep"),
        static_cast<int32>(Best.Kind),
        static_cast<int32>(ESoundKind::Footstep));
    TestFalse(TEXT("none heard"), None.bHeard);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FSoundPropagationAmbientForTest,
    "GTC.Systems.SoundPropagation.AmbientForNightQuieterRainLouderFloor",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSoundPropagationAmbientForTest::RunTest(const FString& Parameters)
{
    const double Base = 0.2;
    const double Night = FSoundPropagation::AmbientFor(Base, true, 0.0);
    const double Day = FSoundPropagation::AmbientFor(Base, false, 0.0);
    const double Rainy = FSoundPropagation::AmbientFor(Base, false, 1.0);
    TestTrue(TEXT("night < day"), Night < Day);
    TestTrue(TEXT("rainy > day"), Rainy > Day);
    TestTrue(TEXT("night >= 0"), Night >= 0.0);
    TestTrue(TEXT("rainy <= 1"), Rainy <= 1.0);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
