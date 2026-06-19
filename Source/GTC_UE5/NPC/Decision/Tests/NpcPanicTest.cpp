// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcPanic.h"
#include "../NpcReaction.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FNpcPanic — graded person-to-person fear contagion and its
 * saturating reinforcement. Prefix GTC.NPC.Decision.NpcPanic.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcPanicTest,
    "GTC.NPC.Decision.NpcPanic",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcPanicTest::RunTest(const FString& Parameters)
{
    const double Rad = FNpcPanic::ContagionRadiusM;
    const double Timid = 0.0; // catches all of it

    // A timid neighbour right next to a fully-panicked source catches ~all the fear.
    TestTrue(TEXT("point-blank timid neighbour catches ~full source fear"),
        FNpcPanic::CaughtFear(1.0, 0.0, Timid, Rad) >= 1.0 - GtcTest::Eps);

    // Distance falloff is monotone: closer always catches at least as much as farther.
    double Prev = FNpcPanic::CaughtFear(1.0, 0.0, Timid, Rad);
    for (double D = 0.5; D < Rad; D += 0.5)
    {
        const double Now = FNpcPanic::CaughtFear(1.0, D, Timid, Rad);
        TestTrue(TEXT("contagion falls off monotonically with distance"), Now <= Prev + GtcTest::Eps);
        Prev = Now;
    }

    // Beyond the contagion radius, terror doesn't jump.
    TestEqual(TEXT("out of range catches nothing"),
        FNpcPanic::CaughtFear(1.0, Rad + 0.01, Timid, Rad), 0.0);

    // The steady-nerved hold the line and stop the wave, even point-blank.
    TestEqual(TEXT("steady nerve catches nothing point-blank"),
        FNpcPanic::CaughtFear(1.0, 0.0, FNpcPanic::SteadyNerve, Rad), 0.0);
    TestEqual(TEXT("nerve above the threshold catches nothing"),
        FNpcPanic::CaughtFear(1.0, 0.0, FNpcPanic::SteadyNerve + 0.1, Rad), 0.0);

    // A barely-rattled source spreads less than a terrified one at the same distance.
    TestTrue(TEXT("calmer source spreads less fear"),
        FNpcPanic::CaughtFear(0.3, 2.0, Timid, Rad) < FNpcPanic::CaughtFear(1.0, 2.0, Timid, Rad));

    // An already-calm source spreads nothing.
    TestEqual(TEXT("calm source spreads nothing"),
        FNpcPanic::CaughtFear(0.0, 0.0, Timid, Rad), 0.0);

    // A braver neighbour catches less than a timid one (same source, same distance).
    TestTrue(TEXT("braver neighbour resists more"),
        FNpcPanic::CaughtFear(1.0, 1.0, 0.6, Rad) < FNpcPanic::CaughtFear(1.0, 1.0, 0.1, Rad));

    // A faint catch below MinShare is treated as nothing (no infinitesimal jitter).
    TestEqual(TEXT("sub-threshold catch floors to zero"),
        FNpcPanic::CaughtFear(0.08, Rad - 0.5, Timid, Rad), 0.0);

    // A non-positive radius is inert.
    TestEqual(TEXT("zero radius is inert"), FNpcPanic::CaughtFear(1.0, 0.0, Timid, 0.0), 0.0);

    // WouldSpread agrees exactly with CaughtFear > 0.
    TestTrue(TEXT("WouldSpread true when fear is caught"),
        FNpcPanic::WouldSpread(1.0, 1.0, Timid, Rad));
    TestFalse(TEXT("WouldSpread false for a steady neighbour"),
        FNpcPanic::WouldSpread(1.0, 1.0, FNpcPanic::SteadyNerve, Rad));
    TestFalse(TEXT("WouldSpread false out of range"),
        FNpcPanic::WouldSpread(1.0, Rad + 1.0, Timid, Rad));

    // Where graded contagion spreads, the binary predecessor agreed it could (proximity
    // + nerve are necessary conditions; the graded model only adds source-fear + MinShare).
    TestTrue(TEXT("graded spread implies the binary CatchesPanic also fired"),
        !FNpcPanic::WouldSpread(1.0, 1.0, 0.1, Rad) || FNpcReaction::CatchesPanic(1.0, 0.1));

    // Reinforce saturates: half-and-half lands at 0.75, never overshoots, and is inert
    // at the extremes.
    TestTrue(TEXT("reinforce(0.5, 0.5) == 0.75"),
        FMath::IsNearlyEqual(FNpcPanic::Reinforce(0.5, 0.5), 0.75, GtcTest::Eps));
    TestTrue(TEXT("reinforce never exceeds 1"),
        FNpcPanic::Reinforce(0.9, 0.9) <= 1.0 + GtcTest::Eps);
    TestTrue(TEXT("reinforce with no catch leaves current fear"),
        FMath::IsNearlyEqual(FNpcPanic::Reinforce(0.4, 0.0), 0.4, GtcTest::Eps));
    TestTrue(TEXT("reinforce from calm equals the catch"),
        FMath::IsNearlyEqual(FNpcPanic::Reinforce(0.0, 0.6), 0.6, GtcTest::Eps));

    // Reinforce is monotone in the caught fear: a bigger nudge never lowers dread.
    TestTrue(TEXT("reinforce is monotone in caught fear"),
        FNpcPanic::Reinforce(0.3, 0.2) <= FNpcPanic::Reinforce(0.3, 0.5) + GtcTest::Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
