// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../AimAssist.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FAimAssist — soft-lock acquisition scoring and aim adhesion.
 * Prefix GTC.Weapons.Targeting.AimAssist.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FAimAssistTest,
    "GTC.Weapons.Targeting.AimAssist",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

static FAimCandidate MakeCandidate(const FVector& Pos, double Priority = 1.0, bool bTargetable = true)
{
    FAimCandidate C;
    C.Position = Pos;
    C.Priority = Priority;
    C.bTargetable = bTargetable;
    return C;
}

bool FAimAssistTest::RunTest(const FString& Parameters)
{
    const FVector Origin = FVector::ZeroVector;
    const FVector Fwd(1.0, 0.0, 0.0);
    const double Cone = FAimAssist::DefaultConeHalfAngleRad;
    const double Range = FAimAssist::DefaultRangeM;

    // --- AngleToTargetRad ---
    TestTrue(TEXT("target dead ahead is ~0 off-axis"),
        FMath::IsNearlyEqual(FAimAssist::AngleToTargetRad(Origin, Fwd, FVector(10, 0, 0)), 0.0, GtcTest::Eps));
    TestTrue(TEXT("target at 90 degrees is ~pi/2"),
        FMath::IsNearlyEqual(FAimAssist::AngleToTargetRad(Origin, Fwd, FVector(0, 10, 0)), PI / 2.0, GtcTest::Eps));
    TestTrue(TEXT("target behind is ~pi"),
        FMath::IsNearlyEqual(FAimAssist::AngleToTargetRad(Origin, Fwd, FVector(-10, 0, 0)), PI, GtcTest::Eps));
    TestEqual(TEXT("target on the origin is inert"),
        FAimAssist::AngleToTargetRad(Origin, Fwd, Origin), 0.0);

    // --- Score gating ---
    TestTrue(TEXT("a centred, in-range, hostile target scores > 0"),
        FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0)), Cone, Range) > 0.0);
    TestEqual(TEXT("a non-targetable candidate scores 0"),
        FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0), 1.0, /*bTargetable*/ false), Cone, Range), 0.0);
    TestEqual(TEXT("a zero-priority candidate scores 0"),
        FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0), 0.0), Cone, Range), 0.0);
    TestEqual(TEXT("a candidate beyond range scores 0"),
        FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(Range + 5.0, 0, 0)), Cone, Range), 0.0);
    // A target just outside the cone (place it well past the half-angle) scores 0.
    TestEqual(TEXT("a candidate outside the cone scores 0"),
        FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 10, 0)), Cone, Range), 0.0);

    // --- Score ordering ---
    // Same range, more centred scores higher.
    const double Centred = FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0)), Cone, Range);
    const double OffAxis = FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 1.0, 0)), Cone, Range);
    TestTrue(TEXT("more centred scores higher than off-axis"), Centred > OffAxis);
    // Same bearing, closer scores higher.
    const double Near = FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0)), Cone, Range);
    const double Far = FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(40, 0, 0)), Cone, Range);
    TestTrue(TEXT("closer scores higher than farther"), Near > Far);
    // Higher priority scores higher, all else equal.
    const double LowPri = FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0), 1.0), Cone, Range);
    const double HiPri = FAimAssist::Score(Origin, Fwd, MakeCandidate(FVector(10, 0, 0), 2.0), Cone, Range);
    TestTrue(TEXT("higher priority scores higher"), HiPri > LowPri);

    // --- SelectBestTarget ---
    {
        TArray<FAimCandidate> Cands;
        Cands.Add(MakeCandidate(FVector(20, 3.0, 0)));  // off-axis
        Cands.Add(MakeCandidate(FVector(15, 0.0, 0)));  // centred + closer -> winner
        Cands.Add(MakeCandidate(FVector(-10, 0.0, 0))); // behind -> ineligible
        TestEqual(TEXT("selects the centred, closer target"),
            FAimAssist::SelectBestTarget(Origin, Fwd, Cands, Cone, Range), 1);
    }
    {
        TArray<FAimCandidate> None;
        None.Add(MakeCandidate(FVector(-10, 0, 0)));            // behind
        None.Add(MakeCandidate(FVector(Range + 50.0, 0, 0)));   // too far
        TestEqual(TEXT("returns INDEX_NONE when nothing is acquirable"),
            FAimAssist::SelectBestTarget(Origin, Fwd, None, Cone, Range), (int32)INDEX_NONE);
        TArray<FAimCandidate> Empty;
        TestEqual(TEXT("empty candidate list -> INDEX_NONE"),
            FAimAssist::SelectBestTarget(Origin, Fwd, Empty, Cone, Range), (int32)INDEX_NONE);
    }
    {
        // A high-priority hostile pips an equally-placed low-priority one.
        TArray<FAimCandidate> Cands;
        Cands.Add(MakeCandidate(FVector(15, 0, 0), 1.0));
        Cands.Add(MakeCandidate(FVector(15, 0, 0), 3.0));
        TestEqual(TEXT("priority breaks an otherwise-equal contest"),
            FAimAssist::SelectBestTarget(Origin, Fwd, Cands, Cone, Range), 1);
    }

    // --- Magnetism ---
    TestTrue(TEXT("magnetism is full dead-centre"),
        FMath::IsNearlyEqual(FAimAssist::Magnetism(0.0, Cone), 1.0, GtcTest::Eps));
    TestEqual(TEXT("magnetism is zero at the cone edge"), FAimAssist::Magnetism(Cone, Cone), 0.0);
    TestEqual(TEXT("magnetism is zero beyond the cone"), FAimAssist::Magnetism(Cone * 2.0, Cone), 0.0);
    TestTrue(TEXT("magnetism decreases with angle"),
        FAimAssist::Magnetism(Cone * 0.25, Cone) > FAimAssist::Magnetism(Cone * 0.75, Cone));

    // --- ApplyAdhesion ---
    {
        const FVector Target(10, 4, 0); // off to the side
        const double Before = FAimAssist::AngleToTargetRad(Origin, Fwd, Target);

        // No strength leaves the aim untouched.
        const FVector Zero = FAimAssist::ApplyAdhesion(Fwd, Origin, Target, 0.0);
        TestTrue(TEXT("zero adhesion leaves the aim unchanged"),
            FMath::IsNearlyEqual(FAimAssist::AngleToTargetRad(Origin, Zero, Target), Before, GtcTest::Eps));

        // Partial strength reduces the residual angle but doesn't snap fully on.
        const FVector Half = FAimAssist::ApplyAdhesion(Fwd, Origin, Target, 0.5);
        const double HalfErr = FAimAssist::AngleToTargetRad(Origin, Half, Target);
        TestTrue(TEXT("partial adhesion pulls the aim closer to target"), HalfErr < Before - GtcTest::Eps);

        // Full strength points straight at the target.
        const FVector Full = FAimAssist::ApplyAdhesion(Fwd, Origin, Target, 1.0);
        TestTrue(TEXT("full adhesion lands on the target"),
            FMath::IsNearlyEqual(FAimAssist::AngleToTargetRad(Origin, Full, Target), 0.0, GtcTest::Eps));

        // More strength => smaller residual angle (monotone).
        TestTrue(TEXT("adhesion is monotone in strength"), HalfErr < Before);
        TestTrue(TEXT("stronger adhesion beats weaker"),
            FAimAssist::AngleToTargetRad(Origin, FAimAssist::ApplyAdhesion(Fwd, Origin, Target, 0.8), Target) < HalfErr);

        // Result is always unit length.
        TestTrue(TEXT("adhesion returns a unit vector"),
            FMath::IsNearlyEqual(Half.Size(), 1.0, GtcTest::Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
