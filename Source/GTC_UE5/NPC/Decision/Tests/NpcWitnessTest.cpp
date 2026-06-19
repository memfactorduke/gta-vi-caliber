// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcWitness.h"
#include "../NpcMemory.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FNpcWitness — the distance falloff of how strongly a bystander
 * registers an assault they witness. Prefix GTC.NPC.Decision.NpcWitness.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcWitnessTest,
    "GTC.NPC.Decision.NpcWitness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcWitnessTest::RunTest(const FString& Parameters)
{
    const double Rad = FNpcWitness::DefaultRadiusM;

    // A witness right on top of the event gets essentially the full intensity.
    TestTrue(TEXT("point-blank witness gets ~full intensity"),
        FNpcWitness::Share(1.0, 0.0, Rad) >= 1.0 - GtcTest::Eps);

    // Falloff is monotone: closer always registers at least as much as farther.
    const double Near = FNpcWitness::Share(1.0, 2.0, Rad);
    const double Mid = FNpcWitness::Share(1.0, 6.0, Rad);
    const double Far = FNpcWitness::Share(1.0, 10.0, Rad);
    TestTrue(TEXT("near >= mid"), Near >= Mid);
    TestTrue(TEXT("mid >= far"), Mid >= Far);
    TestTrue(TEXT("near is strictly more than far"), Near > Far);

    // At and beyond the radius, nothing is registered.
    TestEqual(TEXT("at the radius edge -> 0"), FNpcWitness::Share(1.0, Rad, Rad), 0.0);
    TestEqual(TEXT("beyond the radius -> 0"), FNpcWitness::Share(1.0, Rad + 5.0, Rad), 0.0);

    // A faint event far away falls below MinShare and is dropped to 0 (too distant
    // to care), rather than leaving a sliver of memory.
    TestEqual(TEXT("faint + distant -> dropped to 0"),
        FNpcWitness::Share(0.2, 11.0, Rad), 0.0);

    // Base intensity scales the share: a brutal knockdown alarms a close witness
    // past FNpcMemory::Alarmed; a mild event close by only makes them uneasy.
    TestTrue(TEXT("brutal close event alarms a witness"),
        FNpcWitness::Share(0.9, 1.5, Rad) >= FNpcMemory::Alarmed);
    {
        const double Mild = FNpcWitness::Share(0.35, 1.5, Rad);
        TestTrue(TEXT("mild close event is only uneasy, not alarmed"),
            Mild >= FNpcMemory::Uneasy && Mild < FNpcMemory::Alarmed);
    }

    // Inputs are clamped/guarded: negative distance == point-blank, over-unit base
    // can't exceed 1, non-positive radius yields nothing.
    TestTrue(TEXT("negative distance treated as point-blank"),
        FNpcWitness::Share(1.0, -5.0, Rad) >= 1.0 - GtcTest::Eps);
    TestTrue(TEXT("over-unit base clamps to <= 1"),
        FNpcWitness::Share(5.0, 0.0, Rad) <= 1.0 + GtcTest::Eps);
    TestEqual(TEXT("non-positive radius -> 0"), FNpcWitness::Share(1.0, 1.0, 0.0), 0.0);

    // Exact interior values pin the LINEAR shape (ordering checks alone can't tell
    // linear from quadratic — both are monotone). Three points uniquely fix the line.
    TestTrue(TEXT("linear midpoint 6m -> 0.5"),
        FMath::IsNearlyEqual(FNpcWitness::Share(1.0, 6.0, Rad), 0.5, GtcTest::Eps));
    TestTrue(TEXT("linear near 2m -> 0.8333"),
        FMath::IsNearlyEqual(FNpcWitness::Share(1.0, 2.0, Rad), 5.0 / 6.0, GtcTest::Eps));
    TestTrue(TEXT("linear far 10m -> 0.16667"),
        FMath::IsNearlyEqual(FNpcWitness::Share(1.0, 10.0, Rad), 1.0 / 6.0, GtcTest::Eps));

    // Straddle the MinShare(0.05) cliff at full intensity (it bites at ~11.4m) with a
    // comfortable margin, so a change to MinShare in either direction is caught — the
    // existing faint+distant case only proves a floor exists somewhere, not its value.
    TestTrue(TEXT("just inside MinShare cliff survives (falloff ~0.06)"),
        FNpcWitness::Share(1.0, 11.28, Rad) > 0.0);
    TestEqual(TEXT("just past MinShare cliff drops to 0 (falloff ~0.04)"),
        FNpcWitness::Share(1.0, 11.52, Rad), 0.0);

    // Custom RadiusM must actually be used as the divisor (not hardcoded to the
    // 12 m default): half-radius reads 0.5, the edge reads 0.
    TestTrue(TEXT("half custom radius -> 0.5"),
        FMath::IsNearlyEqual(FNpcWitness::Share(1.0, 10.0, 20.0), 0.5, GtcTest::Eps));
    TestEqual(TEXT("custom radius edge -> 0"), FNpcWitness::Share(1.0, 20.0, 20.0), 0.0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
