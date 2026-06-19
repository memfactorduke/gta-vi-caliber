// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CrimeWitness.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FCrimeWitness::ReportDelayFor — the GTC-original crowd/proximity
 * scaling of the witness report delay (not part of the Godot parity oracle). Prefix
 * GTC.Systems.Wanted.CrimeWitness.ReportDelay.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCrimeWitnessReportDelayTest,
    "GTC.Systems.Wanted.CrimeWitness.ReportDelay",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCrimeWitnessReportDelayTest::RunTest(const FString& Parameters)
{
    const double Base = 6.0;
    const double Sight = 10.0;

    // A lone witness at the edge of sight takes the full base delay.
    TestTrue(TEXT("lone edge witness => full base delay"),
        FMath::IsNearlyEqual(FCrimeWitness::ReportDelayFor(Base, 1, Sight, Sight), Base, GtcTest::Eps));

    // Closer reports faster than farther (same lone witness).
    TestTrue(TEXT("closer witness reports sooner"),
        FCrimeWitness::ReportDelayFor(Base, 1, 1.0, Sight) < FCrimeWitness::ReportDelayFor(Base, 1, Sight, Sight));

    // A crowd reports faster than a lone witness at the same distance.
    TestTrue(TEXT("more witnesses report sooner"),
        FCrimeWitness::ReportDelayFor(Base, 5, 5.0, Sight) < FCrimeWitness::ReportDelayFor(Base, 1, 5.0, Sight));

    // The delay never drops below the floor, however large/close the crowd.
    TestTrue(TEXT("never below the floor"),
        FCrimeWitness::ReportDelayFor(Base, 50, 0.0, Sight) >= FCrimeWitness::MinReportDelay - GtcTest::Eps);

    // Witness count is clamped to >= 1 (0 or negative behaves like a lone witness).
    TestTrue(TEXT("zero witnesses clamps to a lone witness"),
        FMath::IsNearlyEqual(
            FCrimeWitness::ReportDelayFor(Base, 0, Sight, Sight),
            FCrimeWitness::ReportDelayFor(Base, 1, Sight, Sight), GtcTest::Eps));

    // Degenerate inputs pass the base delay through unscaled.
    TestTrue(TEXT("non-positive sight range is unscaled"),
        FMath::IsNearlyEqual(FCrimeWitness::ReportDelayFor(Base, 5, 1.0, 0.0), Base, GtcTest::Eps));
    TestTrue(TEXT("non-positive base delay is unscaled"),
        FMath::IsNearlyEqual(FCrimeWitness::ReportDelayFor(0.0, 5, 1.0, Sight), 0.0, GtcTest::Eps));

    // It composes with the stateful report: a close crowd's report lands before a lone
    // distant witness's would have.
    {
        const double FastDelay = FCrimeWitness::ReportDelayFor(Base, 6, 1.0, Sight);
        FCrimeWitness Report(FastDelay);
        TestFalse(TEXT("not reported before its (short) delay"), Report.IsReported());
        Report.Tick(FastDelay + 0.01);
        TestTrue(TEXT("crowd report lands after its scaled delay"), Report.IsReported());

        // The same crime, silenced in time, never reports.
        FCrimeWitness Silenced(FastDelay);
        Silenced.Silence();
        Silenced.Tick(FastDelay + 100.0);
        TestFalse(TEXT("silenced crowd report never lands"), Silenced.IsReported());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
