// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PartnerBond.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FPartnerBond — the two-lead relationship meter. GTC-original (no Godot
 * oracle). Pins the first warming gain, the diminishing returns, the flat cooling
 * loss, the [0,1] clamps, neglect relaxing toward the baseline (both down from high
 * and up from low), the tier thresholds, and Strength/Dt clamping. Prefix
 * GTC.Systems.Roster.Bond.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace PartnerBondTest
{
    FPartnerBond::FParams Tuning()
    {
        FPartnerBond::FParams P; // start/baseline 0.2, warm 0.15, cool 0.20, decay 0.01/hr,
                                 // tiers 0.25 / 0.50 / 0.80
        return P;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPartnerBondTest,
    "GTC.Systems.Roster.Bond",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPartnerBondTest::RunTest(const FString& Parameters)
{
    using namespace PartnerBondTest;
    using ETier = FPartnerBond::ETier;

    // ---- Fresh bond starts estranged ------------------------------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        TestTrue(TEXT("starts at 0.2"), FMath::IsNearlyEqual(B.Bond(), 0.2, Eps));
        TestTrue(TEXT("starts Estranged"), B.Tier() == ETier::Estranged);
    }

    // ---- Warming raises the bond, with diminishing returns --------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        const double First = B.Warm(1.0); // 0.15 * (1 - 0.2) = 0.12
        TestTrue(TEXT("first warm gains 0.12"), FMath::IsNearlyEqual(First, 0.12, Eps));
        TestTrue(TEXT("bond now 0.32"), FMath::IsNearlyEqual(B.Bond(), 0.32, Eps));
        TestTrue(TEXT("now Wary"), B.Tier() == ETier::Wary);

        const double Second = B.Warm(1.0); // 0.15 * (1 - 0.32) = 0.102
        TestTrue(TEXT("second warm gains less (diminishing)"), Second < First);
    }

    // ---- Cooling takes a flat chunk -------------------------------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        B.Warm(1.0); // 0.32
        const double Change = B.Cool(1.0); // flat -0.20
        TestTrue(TEXT("cooling is a flat -0.2"), FMath::IsNearlyEqual(Change, -0.2, Eps));
        TestTrue(TEXT("bond back to 0.12"), FMath::IsNearlyEqual(B.Bond(), 0.12, Eps));
    }

    // ---- Bond clamps to [0,1] -------------------------------------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        for (int32 I = 0; I < 200; ++I)
        {
            B.Warm(1.0);
        }
        TestTrue(TEXT("warming asymptotes to ~1"), FMath::IsNearlyEqual(B.Bond(), 1.0, 1e-2));
        TestTrue(TEXT("never exceeds 1"), B.Bond() <= 1.0);
        TestTrue(TEXT("maxed bond is RideOrDie"), B.Tier() == ETier::RideOrDie);

        for (int32 I = 0; I < 200; ++I)
        {
            B.Cool(1.0);
        }
        TestTrue(TEXT("cooling floors at 0"), FMath::Abs(B.Bond()) <= Eps);
        TestTrue(TEXT("never goes negative"), B.Bond() >= 0.0);
    }

    // ---- Neglect relaxes a high bond down toward the baseline ------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        for (int32 I = 0; I < 10; ++I)
        {
            B.Warm(1.0); // climb well above baseline
        }
        const double High = B.Bond();
        TestTrue(TEXT("climbed above baseline"), High > 0.2);
        B.Advance(50.0); // 0.01/hr * 50 = close half the gap to 0.2
        TestTrue(TEXT("neglect cools toward baseline"), B.Bond() < High);
        TestTrue(TEXT("but not below the baseline"), B.Bond() > 0.2 - Eps);
    }

    // ---- Neglect mends a damaged bond up toward the baseline -------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        B.Cool(1.0); // 0.2 -> 0.0 (below baseline)
        TestTrue(TEXT("dropped below baseline"), B.Bond() < 0.2);
        B.Advance(50.0);
        TestTrue(TEXT("time mends toward baseline"), B.Bond() > 0.0);
        TestTrue(TEXT("but not above the baseline"), B.Bond() < 0.2 + Eps);
    }

    // ---- At the baseline, neglect changes nothing -----------------------------
    {
        FPartnerBond B;
        B.Configure(Tuning()); // bond == baseline 0.2
        B.Advance(100.0);
        TestTrue(TEXT("baseline bond is stable"), FMath::IsNearlyEqual(B.Bond(), 0.2, Eps));
    }

    // ---- Tier ladder ----------------------------------------------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        TestTrue(TEXT("0.2 Estranged"), B.Tier() == ETier::Estranged);
        B.Warm(1.0); // 0.32
        TestTrue(TEXT("0.32 Wary"), B.Tier() == ETier::Wary);
        B.Warm(1.0);
        B.Warm(1.0); // ~0.509
        TestTrue(TEXT(">=0.5 Solid"), B.Tier() == ETier::Solid);
        for (int32 I = 0; I < 20; ++I)
        {
            B.Warm(1.0);
        }
        TestTrue(TEXT(">=0.8 RideOrDie"), B.Tier() == ETier::RideOrDie);
    }

    // ---- Strength and Dt clamps ----------------------------------------------
    {
        FPartnerBond B;
        B.Configure(Tuning());
        TestTrue(TEXT("negative warm strength does nothing"), FMath::Abs(B.Warm(-5.0)) <= Eps);
        TestTrue(TEXT("negative cool strength does nothing"), FMath::Abs(B.Cool(-5.0)) <= Eps);
        const double Held = B.Bond();
        B.Advance(-10.0);
        TestTrue(TEXT("negative Dt does nothing"), FMath::IsNearlyEqual(B.Bond(), Held, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
