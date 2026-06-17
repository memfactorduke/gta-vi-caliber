// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StealthDetection.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Each test below maps 1:1 to a test_* function in the the reference reference behavior
// game/tests/unit/test_stealth_detection.gd.

// test_starts_unaware_at_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionStartsUnawareTest,
    "GTC.NPC.Stealth.StartsUnawareAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionStartsUnawareTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    TestEqual(TEXT("awareness 0"), D.Awareness(), 0.0, Eps);
    TestEqual(
        TEXT("state Unaware"), static_cast<int32>(D.State()), static_cast<int32>(EStealthState::Unaware));
    TestFalse(TEXT("not alerted"), D.IsAlerted());
    TestFalse(TEXT("not suspicious"), D.IsSuspicious());
    return true;
}

// test_seeing_fills_awareness
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionSeeingFillsTest,
    "GTC.NPC.Stealth.SeeingFillsAwareness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionSeeingFillsTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 1.0);
    TestEqual(TEXT("awareness 0.5"), D.Awareness(), 0.5, Eps);
    return true;
}

// test_fill_scaled_by_visibility
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionFillScaledByVisibilityTest,
    "GTC.NPC.Stealth.FillScaledByVisibility",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionFillScaledByVisibilityTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 0.5, 1.0);
    TestEqual(TEXT("awareness 0.25"), D.Awareness(), 0.25, Eps);
    return true;
}

// test_crossing_threshold_is_suspicious
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionCrossingThresholdSuspiciousTest,
    "GTC.NPC.Stealth.CrossingThresholdIsSuspicious",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionCrossingThresholdSuspiciousTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25, 0.4);
    D.Update(true, 1.0, 1.0); // 0.5, above 0.4 threshold, below 1.0
    TestEqual(
        TEXT("state Suspicious"),
        static_cast<int32>(D.State()),
        static_cast<int32>(EStealthState::Suspicious));
    TestTrue(TEXT("is suspicious"), D.IsSuspicious());
    return true;
}

// test_below_threshold_stays_unaware
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionBelowThresholdUnawareTest,
    "GTC.NPC.Stealth.BelowThresholdStaysUnaware",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionBelowThresholdUnawareTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25, 0.4);
    D.Update(true, 1.0, 0.5); // 0.25, below 0.4
    TestEqual(
        TEXT("state Unaware"), static_cast<int32>(D.State()), static_cast<int32>(EStealthState::Unaware));
    return true;
}

// test_reaching_one_alerts
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionReachingOneAlertsTest,
    "GTC.NPC.Stealth.ReachingOneAlerts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionReachingOneAlertsTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 2.0); // fill 0.5/s * 2s = 1.0
    TestEqual(TEXT("awareness 1.0"), D.Awareness(), 1.0, Eps);
    TestTrue(TEXT("is alerted"), D.IsAlerted());
    TestEqual(
        TEXT("state Alerted"), static_cast<int32>(D.State()), static_cast<int32>(EStealthState::Alerted));
    return true;
}

// test_alerted_is_sticky_after_sight_lost
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionAlertedStickyTest,
    "GTC.NPC.Stealth.AlertedIsStickyAfterSightLost",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionAlertedStickyTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 2.0);   // -> alerted
    D.Update(false, 0.0, 10.0); // lots of no-sight time
    TestTrue(TEXT("still alerted"), D.IsAlerted());
    TestEqual(TEXT("awareness pinned 1.0"), D.Awareness(), 1.0, Eps);
    return true;
}

// test_alerted_not_suspicious
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionAlertedNotSuspiciousTest,
    "GTC.NPC.Stealth.AlertedNotSuspicious",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionAlertedNotSuspiciousTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 2.0);
    TestTrue(TEXT("is alerted"), D.IsAlerted());
    TestFalse(TEXT("not suspicious"), D.IsSuspicious());
    return true;
}

// test_not_seen_decays_awareness
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionNotSeenDecaysTest,
    "GTC.NPC.Stealth.NotSeenDecaysAwareness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionNotSeenDecaysTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 1.0);  // 0.5
    D.Update(false, 0.0, 1.0); // -0.25 -> 0.25
    TestEqual(TEXT("awareness 0.25"), D.Awareness(), 0.25, Eps);
    return true;
}

// test_decays_back_to_unaware
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionDecaysBackUnawareTest,
    "GTC.NPC.Stealth.DecaysBackToUnaware",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionDecaysBackUnawareTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25, 0.4);
    D.Update(true, 1.0, 1.0);   // 0.5, suspicious
    D.Update(false, 0.0, 10.0); // decays to 0
    TestEqual(TEXT("awareness 0"), D.Awareness(), 0.0, Eps);
    TestEqual(
        TEXT("state Unaware"), static_cast<int32>(D.State()), static_cast<int32>(EStealthState::Unaware));
    return true;
}

// test_decay_floors_at_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionDecayFloorsTest,
    "GTC.NPC.Stealth.DecayFloorsAtZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionDecayFloorsTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 0.5);    // 0.25
    D.Update(false, 0.0, 100.0); // would go negative
    TestEqual(TEXT("awareness floored 0"), D.Awareness(), 0.0, Eps);
    return true;
}

// test_visibility_zero_does_not_fill
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionVisibilityZeroNoFillTest,
    "GTC.NPC.Stealth.VisibilityZeroDoesNotFill",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionVisibilityZeroNoFillTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 0.0, 5.0); // can see, but can't make out
    TestEqual(TEXT("awareness 0"), D.Awareness(), 0.0, Eps);
    TestEqual(
        TEXT("state Unaware"), static_cast<int32>(D.State()), static_cast<int32>(EStealthState::Unaware));
    return true;
}

// test_awareness_clamped_at_one
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionAwarenessClampedTest,
    "GTC.NPC.Stealth.AwarenessClampedAtOne",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionAwarenessClampedTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 100.0); // massively over 1.0
    TestEqual(TEXT("awareness clamped 1.0"), D.Awareness(), 1.0, Eps);
    return true;
}

// test_negative_delta_ignored
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionNegativeDeltaIgnoredTest,
    "GTC.NPC.Stealth.NegativeDeltaIgnored",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionNegativeDeltaIgnoredTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 1.0);    // 0.5
    D.Update(true, 1.0, -5.0);   // ignored
    D.Update(false, 0.0, -5.0);  // ignored
    TestEqual(TEXT("awareness unchanged 0.5"), D.Awareness(), 0.5, Eps);
    return true;
}

// test_detection_speed_falls_with_distance
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionSpeedFallsDistanceTest,
    "GTC.NPC.Stealth.DetectionSpeedFallsWithDistance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionSpeedFallsDistanceTest::RunTest(const FString& Parameters)
{
    const double Near = FStealthDetection::DetectionSpeed(2.0, 10.0, false, false);
    const double Far = FStealthDetection::DetectionSpeed(8.0, 10.0, false, false);
    TestTrue(TEXT("near > far"), Near > Far);
    TestEqual(TEXT("near 0.8"), Near, 0.8, Eps);
    TestEqual(TEXT("far 0.2"), Far, 0.2, Eps);
    return true;
}

// test_detection_speed_out_of_range_zero
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionSpeedOutOfRangeTest,
    "GTC.NPC.Stealth.DetectionSpeedOutOfRangeZero",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionSpeedOutOfRangeTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("at range == 0"), FStealthDetection::DetectionSpeed(10.0, 10.0, false, false), 0.0, Eps);
    TestEqual(
        TEXT("beyond range == 0"), FStealthDetection::DetectionSpeed(15.0, 10.0, false, false), 0.0, Eps);
    TestEqual(
        TEXT("zero range == 0"), FStealthDetection::DetectionSpeed(1.0, 0.0, false, false), 0.0, Eps);
    return true;
}

// test_detection_speed_crouch_lowers
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionSpeedCrouchLowersTest,
    "GTC.NPC.Stealth.DetectionSpeedCrouchLowers",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionSpeedCrouchLowersTest::RunTest(const FString& Parameters)
{
    const double Standing = FStealthDetection::DetectionSpeed(2.0, 10.0, false, false);
    const double Crouched = FStealthDetection::DetectionSpeed(2.0, 10.0, true, false);
    TestTrue(TEXT("crouched < standing"), Crouched < Standing);
    TestEqual(TEXT("crouched 0.8*0.45"), Crouched, 0.8 * 0.45, Eps);
    return true;
}

// test_detection_speed_moving_raises_and_clamps
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionSpeedMovingRaisesTest,
    "GTC.NPC.Stealth.DetectionSpeedMovingRaisesAndClamps",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionSpeedMovingRaisesTest::RunTest(const FString& Parameters)
{
    const double Still = FStealthDetection::DetectionSpeed(5.0, 10.0, false, false);
    const double Moving = FStealthDetection::DetectionSpeed(5.0, 10.0, false, true);
    // still = 0.5, moving = 0.5 * 1.4 = 0.7 (in range, not clamped)
    const double NearMoving = FStealthDetection::DetectionSpeed(1.0, 10.0, false, true); // 0.9*1.4=1.26 -> 1.0
    TestTrue(TEXT("moving > still"), Moving > Still);
    TestEqual(TEXT("moving 0.7"), Moving, 0.7, Eps);
    TestEqual(TEXT("near moving clamped 1.0"), NearMoving, 1.0, Eps);
    return true;
}

// test_reset_clears_everything
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthDetectionResetClearsTest,
    "GTC.NPC.Stealth.ResetClearsEverything",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FStealthDetectionResetClearsTest::RunTest(const FString& Parameters)
{
    FStealthDetection D(0.5, 0.25);
    D.Update(true, 1.0, 2.0); // alerted
    D.Reset();
    TestEqual(TEXT("awareness 0"), D.Awareness(), 0.0, Eps);
    TestFalse(TEXT("not alerted"), D.IsAlerted());
    TestEqual(
        TEXT("state Unaware"), static_cast<int32>(D.State()), static_cast<int32>(EStealthState::Unaware));
    return true;
}

#endif // WITH_AUTOMATION_TESTS
