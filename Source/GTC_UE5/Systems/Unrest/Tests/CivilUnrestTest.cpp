// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CivilUnrest.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FCivilUnrest — the district disorder tipping point. GTC-original (no
 * Godot oracle). Pins the calm baseline, sub-flashpoint provocation calming on its
 * own, a provocation past the flashpoint self-sustaining and climbing, suppression
 * having to push BELOW the flashpoint to break a riot (the hysteresis), the stage
 * thresholds, clamps, and negative-Dt. Prefix GTC.Systems.Unrest. All inputs are
 * method calls, so no shared helpers.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCivilUnrestTest,
    "GTC.Systems.Unrest",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCivilUnrestTest::RunTest(const FString& Parameters)
{
    using EStage = FCivilUnrest::EStage;
    const FCivilUnrest::FParams P; // flashpoint 0.6, tense 0.3, decay 0.05/s, growth 0.04/s

    // ---- A calm district stays calm -------------------------------------------
    {
        FCivilUnrest D;
        D.Configure(P);
        TestTrue(TEXT("starts calm"), D.Stage() == EStage::Calm);
        TestFalse(TEXT("not rioting"), D.IsRioting());
        D.Advance(10.0);
        TestTrue(TEXT("stays at zero"), FMath::Abs(D.Unrest()) <= Eps);
    }

    // ---- A scuffle below the flashpoint settles on its own --------------------
    {
        FCivilUnrest D;
        D.Configure(P);
        D.Provoke(0.4); // tense, but under the flashpoint
        TestTrue(TEXT("provoked to 0.4"), FMath::IsNearlyEqual(D.Unrest(), 0.4, Eps));
        TestTrue(TEXT("tense"), D.Stage() == EStage::Tense);
        D.Advance(1.0); // 0.4 < 0.6 -> decays
        TestTrue(TEXT("calms toward zero"), FMath::IsNearlyEqual(D.Unrest(), 0.35, Eps));
        D.Advance(100.0);
        TestTrue(TEXT("settles back to calm"), D.Stage() == EStage::Calm);
        TestTrue(TEXT("unrest gone"), FMath::Abs(D.Unrest()) <= Eps);
    }

    // ---- Past the flashpoint it feeds itself ----------------------------------
    {
        FCivilUnrest D;
        D.Configure(P);
        D.Provoke(0.7); // over the flashpoint
        TestTrue(TEXT("rioting"), D.IsRioting());
        TestTrue(TEXT("stage Rioting"), D.Stage() == EStage::Rioting);
        D.Advance(1.0); // self-sustains -> grows
        TestTrue(TEXT("grows without more provocation"), FMath::IsNearlyEqual(D.Unrest(), 0.74, Eps));
        D.Advance(100.0); // climbs to the cap
        TestTrue(TEXT("climbs to a full riot"), FMath::IsNearlyEqual(D.Unrest(), 1.0, Eps));
    }

    // ---- Hysteresis: suppression must cross BELOW the flashpoint to break it ---
    {
        FCivilUnrest D;
        D.Configure(P);
        D.Provoke(0.7);
        D.Advance(1.0); // 0.74, rioting

        D.Suppress(0.05); // 0.69 — still above the flashpoint
        TestTrue(TEXT("a light touch doesn't break the riot"), D.IsRioting());
        D.Advance(1.0);
        TestTrue(TEXT("under-suppression lets it grow again"), D.Unrest() > 0.69);

        // Now actually push it under the line.
        D.Suppress(0.2); // from ~0.73 down to ~0.53
        TestFalse(TEXT("pushed below the flashpoint -> no longer rioting"), D.IsRioting());
        D.Advance(1.0); // now it decays
        TestTrue(TEXT("a broken riot finally calms"), D.Unrest() < 0.53);
    }

    // ---- Clamps + negative Dt -------------------------------------------------
    {
        FCivilUnrest D;
        D.Configure(P);
        D.Provoke(5.0); // clamps to 1
        TestTrue(TEXT("provoke clamps to 1"), FMath::IsNearlyEqual(D.Unrest(), 1.0, Eps));
        D.Suppress(5.0); // clamps to 0
        TestTrue(TEXT("suppress clamps to 0"), FMath::Abs(D.Unrest()) <= Eps);
        D.Provoke(-1.0); // negative -> no-op
        TestTrue(TEXT("negative provoke does nothing"), FMath::Abs(D.Unrest()) <= Eps);

        D.Provoke(0.7);
        const double Before = D.Unrest();
        D.Advance(-5.0); // negative dt -> no-op
        TestTrue(TEXT("negative Dt doesn't advance"), FMath::IsNearlyEqual(D.Unrest(), Before, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
