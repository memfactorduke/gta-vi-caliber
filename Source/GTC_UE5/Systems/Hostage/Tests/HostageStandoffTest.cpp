// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../HostageStandoff.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FHostageStandoff — the human-shield micro-loop. GTC-original (no Godot
 * oracle). Pins the grab, the rising struggle (faster while moving), the break-free,
 * intimidation with geometric falloff, release, terminal states, re-grab, and the
 * dt/holding guards. Prefix GTC.Systems.Hostage. All inputs are method calls, so no
 * shared helpers.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FHostageStandoffTest,
    "GTC.Systems.Hostage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FHostageStandoffTest::RunTest(const FString& Parameters)
{
    using EState = FHostageStandoff::EState;
    const FHostageStandoff::FParams P; // struggle 0.06/s, move x2, relief 0.4, falloff 0.6

    // ---- No hostage to start --------------------------------------------------
    {
        FHostageStandoff H;
        H.Configure(P);
        TestTrue(TEXT("no hostage"), H.State() == EState::None);
        TestFalse(TEXT("no shield"), H.IsShieldActive());
        TestTrue(TEXT("no struggle"), FMath::Abs(H.StruggleFraction()) <= Eps);
    }

    // ---- Grab raises the shield -----------------------------------------------
    {
        FHostageStandoff H;
        H.Configure(P);
        H.Grab();
        TestTrue(TEXT("holding"), H.IsHolding());
        TestTrue(TEXT("shield active"), H.IsShieldActive());
        TestTrue(TEXT("struggle starts at 0"), FMath::Abs(H.StruggleFraction()) <= Eps);
    }

    // ---- Struggle rises, faster while dragging the hostage ---------------------
    {
        FHostageStandoff Still;
        Still.Configure(P);
        Still.Grab();
        Still.Update(false, 5.0); // 0.06 * 5 = 0.3
        TestTrue(TEXT("standing struggle 0.3"), FMath::IsNearlyEqual(Still.StruggleFraction(), 0.3, Eps));

        FHostageStandoff Moving;
        Moving.Configure(P);
        Moving.Grab();
        Moving.Update(true, 5.0); // 0.06 * 2 * 5 = 0.6
        TestTrue(TEXT("moving struggle 0.6"), FMath::IsNearlyEqual(Moving.StruggleFraction(), 0.6, Eps));
        TestTrue(TEXT("moving struggles faster"), Moving.StruggleFraction() > Still.StruggleFraction());
    }

    // ---- The hostage eventually breaks free -----------------------------------
    {
        FHostageStandoff H;
        H.Configure(P);
        H.Grab();
        H.Update(false, 20.0); // 1.2 -> capped, break free
        TestTrue(TEXT("broke free"), H.State() == EState::BrokeFree);
        TestFalse(TEXT("shield gone"), H.IsShieldActive());
    }

    // ---- Intimidation pushes struggle back, with geometric falloff ------------
    {
        FHostageStandoff H;
        H.Configure(P);
        H.Grab();
        H.Update(false, 15.0); // struggle 0.9
        const double R1 = H.Intimidate(); // 0.4
        const double R2 = H.Intimidate(); // 0.4 * 0.6 = 0.24
        const double R3 = H.Intimidate(); // 0.4 * 0.36 = 0.144
        TestTrue(TEXT("first relief 0.4"), FMath::IsNearlyEqual(R1, 0.4, Eps));
        TestTrue(TEXT("second relief 0.24"), FMath::IsNearlyEqual(R2, 0.24, Eps));
        TestTrue(TEXT("third relief 0.144"), FMath::IsNearlyEqual(R3, 0.144, Eps));
        TestTrue(TEXT("each shout buys less"), R1 > R2 && R2 > R3);
        TestEqual(TEXT("three intimidations counted"), H.Intimidations(), 3);
    }

    // ---- Release / terminal states / re-grab ----------------------------------
    {
        FHostageStandoff H;
        H.Configure(P);
        H.Grab();
        H.Release();
        TestTrue(TEXT("released"), H.State() == EState::Released);
        TestFalse(TEXT("no shield once released"), H.IsShieldActive());

        H.Update(true, 100.0); // no-op while released
        TestTrue(TEXT("released is terminal under Update"), H.State() == EState::Released);
        TestTrue(TEXT("intimidating a released hostage does nothing"), FMath::Abs(H.Intimidate()) <= Eps);

        H.Grab(); // grab a fresh one
        TestTrue(TEXT("can grab again"), H.IsHolding());
        TestTrue(TEXT("fresh struggle"), FMath::Abs(H.StruggleFraction()) <= Eps);
        TestEqual(TEXT("intimidation history reset"), H.Intimidations(), 0);
    }

    // ---- Guards: no struggle without a hostage, dt clamp -----------------------
    {
        FHostageStandoff H;
        H.Configure(P);
        H.Update(true, 10.0); // no hostage -> no-op
        TestTrue(TEXT("no struggle without a hostage"), H.State() == EState::None);

        H.Grab();
        H.Update(false, 0.0);  // zero dt
        H.Update(false, -5.0); // negative dt
        TestTrue(TEXT("non-positive dt makes no struggle"), FMath::Abs(H.StruggleFraction()) <= Eps);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
