// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleInsurance.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleInsurance — insure/claim/recover. GTC-original (no Godot
 * oracle). Pins insuring, the latched claimable wreck (idempotent, no double-claim),
 * the first-claim-no-wait + cooldown gate, the escalating deductible, cancel/re-insure,
 * unknown/uninsured rejection, idempotent insure, and per-vehicle independence.
 * Prefix GTC.Vehicles.Insurance.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace VehicleInsuranceTest
{
    const FString Coupe = TEXT("player_coupe");
    const FString Bike = TEXT("player_bike");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleInsuranceTest,
    "GTC.Vehicles.Insurance",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleInsuranceTest::RunTest(const FString& Parameters)
{
    using namespace VehicleInsuranceTest;
    const FVehicleInsurance::FParams P; // base 500, escalation 0.5, cooldown 24h

    // ---- Insure, destroy, claim -----------------------------------------------
    {
        FVehicleInsurance I;
        I.Configure(P);
        TestEqual(TEXT("no policies yet"), I.PolicyCount(), 0);

        I.Insure(Coupe);
        TestTrue(TEXT("now insured"), I.IsInsured(Coupe));
        TestFalse(TEXT("nothing to claim yet"), I.IsClaimable(Coupe));
        TestTrue(TEXT("first quote is base 500"), FMath::IsNearlyEqual(I.QuoteDeductible(Coupe), 500.0, Eps));

        I.ReportDestroyed(Coupe);
        TestTrue(TEXT("wreck is claimable"), I.IsClaimable(Coupe));

        const FVehicleInsurance::FClaimResult R = I.Claim(Coupe, 0.0);
        TestTrue(TEXT("first claim granted"), R.bGranted);
        TestTrue(TEXT("first deductible 500"), FMath::IsNearlyEqual(R.Deductible, 500.0, Eps));
        TestFalse(TEXT("wreck cleared after claim"), I.IsClaimable(Coupe));
        TestEqual(TEXT("one claim made"), I.ClaimsMade(Coupe), 1);
        TestTrue(TEXT("still insured after claim"), I.IsInsured(Coupe));
    }

    // ---- A wreck can't be double-claimed (latched, idempotent) -----------------
    {
        FVehicleInsurance I;
        I.Configure(P);
        I.Insure(Coupe);
        I.ReportDestroyed(Coupe);
        I.ReportDestroyed(Coupe); // duplicate destroy event
        TestTrue(TEXT("granted once"), I.Claim(Coupe, 0.0).bGranted);
        TestFalse(TEXT("second claim on the same wreck denied"), I.Claim(Coupe, 0.0).bGranted);
        TestEqual(TEXT("still just one claim"), I.ClaimsMade(Coupe), 1);
    }

    // ---- Cooldown gates the second claim; deductible escalates -----------------
    {
        FVehicleInsurance I;
        I.Configure(P);
        I.Insure(Coupe);
        I.ReportDestroyed(Coupe);
        I.Claim(Coupe, 0.0); // first claim at hour 0

        I.ReportDestroyed(Coupe); // wrecked again
        const FVehicleInsurance::FClaimResult Early = I.Claim(Coupe, 10.0); // 10h < 24h
        TestFalse(TEXT("claim within cooldown denied"), Early.bGranted);
        TestTrue(TEXT("still claimable after a denied claim"), I.IsClaimable(Coupe));

        const FVehicleInsurance::FClaimResult Late = I.Claim(Coupe, 24.0); // exactly the cooldown
        TestTrue(TEXT("claim after cooldown granted"), Late.bGranted);
        TestTrue(TEXT("second deductible escalates to 750"), FMath::IsNearlyEqual(Late.Deductible, 750.0, Eps));
        TestEqual(TEXT("two claims made"), I.ClaimsMade(Coupe), 2);
        TestTrue(TEXT("third quote is 1000"), FMath::IsNearlyEqual(I.QuoteDeductible(Coupe), 1000.0, Eps));
    }

    // ---- Cancel suspends coverage; re-insuring restores a pending wreck --------
    {
        FVehicleInsurance I;
        I.Configure(P);
        I.Insure(Coupe);
        I.ReportDestroyed(Coupe);
        I.Cancel(Coupe);
        TestFalse(TEXT("cancelled is not insured"), I.IsInsured(Coupe));
        TestFalse(TEXT("cancelled can't claim"), I.IsClaimable(Coupe));
        TestFalse(TEXT("claim denied while cancelled"), I.Claim(Coupe, 100.0).bGranted);

        I.Insure(Coupe); // re-insure
        TestTrue(TEXT("re-insured restores the pending wreck"), I.IsClaimable(Coupe));
        TestTrue(TEXT("can claim again after re-insuring"), I.Claim(Coupe, 100.0).bGranted);
    }

    // ---- Unknown / uninsured vehicles are rejected ----------------------------
    {
        FVehicleInsurance I;
        I.Configure(P);
        I.ReportDestroyed(Coupe); // unknown -> no policy created
        TestEqual(TEXT("destroying an unknown car makes no policy"), I.PolicyCount(), 0);
        TestFalse(TEXT("unknown is not claimable"), I.IsClaimable(Coupe));
        TestFalse(TEXT("claim on unknown denied"), I.Claim(Coupe, 0.0).bGranted);
        TestTrue(TEXT("quote for unknown is 0"), FMath::Abs(I.QuoteDeductible(Coupe)) <= Eps);
    }

    // ---- Idempotent insure + per-vehicle independence -------------------------
    {
        FVehicleInsurance I;
        I.Configure(P);
        I.Insure(Coupe);
        I.Insure(Coupe); // idempotent
        I.Insure(Bike);
        TestEqual(TEXT("two distinct policies"), I.PolicyCount(), 2);

        I.ReportDestroyed(Coupe);
        I.Claim(Coupe, 0.0);
        TestEqual(TEXT("coupe claimed once"), I.ClaimsMade(Coupe), 1);
        TestEqual(TEXT("bike untouched"), I.ClaimsMade(Bike), 0);
        TestFalse(TEXT("bike not claimable"), I.IsClaimable(Bike));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
