// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../StealthTakedown.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FStealthTakedown — rear-arc geometry, reach, execution, and silence.
 * Prefix GTC.NPC.Stealth.Takedown.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStealthTakedownTest,
    "GTC.NPC.Stealth.Takedown",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStealthTakedownTest::RunTest(const FString& Parameters)
{
    const FVector TargetPos(0, 0, 0);
    const FVector Facing(1, 0, 0); // target looks +X

    // Positions relative to a target facing +X:
    const FVector Behind(-100, 0, 0); // directly behind
    const FVector Front(100, 0, 0);   // dead ahead
    const FVector Side(0, 100, 0);    // straight to the side

    // --- RearAngleRad ---
    TestTrue(TEXT("directly behind -> ~0 rear angle"),
        FMath::IsNearlyEqual(FStealthTakedown::RearAngleRad(Facing, TargetPos, Behind), 0.0, GtcTest::Eps));
    TestTrue(TEXT("dead ahead -> ~pi rear angle"),
        FMath::IsNearlyEqual(FStealthTakedown::RearAngleRad(Facing, TargetPos, Front), PI, GtcTest::Eps));
    TestTrue(TEXT("degenerate facing treated as in front"),
        FMath::IsNearlyEqual(FStealthTakedown::RearAngleRad(FVector::ZeroVector, TargetPos, Behind), PI, GtcTest::Eps));

    // --- IsBehind ---
    TestTrue(TEXT("attacker behind is behind"), FStealthTakedown::IsBehind(Facing, TargetPos, Behind));
    TestFalse(TEXT("attacker in front is not behind"), FStealthTakedown::IsBehind(Facing, TargetPos, Front));
    TestFalse(TEXT("attacker at the side is outside the rear arc"),
        FStealthTakedown::IsBehind(Facing, TargetPos, Side));

    // --- InReach ---
    TestTrue(TEXT("close enough is in reach"),
        FStealthTakedown::InReach(Behind, TargetPos, FStealthTakedown::DefaultReach));
    TestFalse(TEXT("too far is out of reach"),
        FStealthTakedown::InReach(FVector(-500, 0, 0), TargetPos, FStealthTakedown::DefaultReach));

    // --- CanExecute: in reach + not alerted, any angle ---
    TestTrue(TEXT("can take down an unaware target from behind"),
        FStealthTakedown::CanExecute(Behind, TargetPos, Facing, /*alerted*/ false));
    TestTrue(TEXT("can take down an unaware target from the side too"),
        FStealthTakedown::CanExecute(Side, TargetPos, Facing, false));
    TestFalse(TEXT("cannot take down an alerted target"),
        FStealthTakedown::CanExecute(Behind, TargetPos, Facing, /*alerted*/ true));
    TestFalse(TEXT("cannot take down out of reach"),
        FStealthTakedown::CanExecute(FVector(-500, 0, 0), TargetPos, Facing, false));

    // --- IsSilent: only from behind on an unaware target ---
    TestTrue(TEXT("behind + unaware is silent"),
        FStealthTakedown::IsSilent(Behind, TargetPos, Facing, false));
    TestFalse(TEXT("from the side is loud (even if unaware)"),
        FStealthTakedown::IsSilent(Side, TargetPos, Facing, false));
    TestFalse(TEXT("from the front is loud"),
        FStealthTakedown::IsSilent(Front, TargetPos, Facing, false));
    TestFalse(TEXT("an alerted target is never a silent takedown"),
        FStealthTakedown::IsSilent(Behind, TargetPos, Facing, /*alerted*/ true));

    return true;
}

#endif // WITH_AUTOMATION_TESTS
