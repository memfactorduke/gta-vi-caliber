// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../MoneyLaundering.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FMoneyLaundering — the dirty-cash wash. GTC-original (no Godot oracle).
 * Pins deposit + capacity ceiling, the throughput-limited per-tick launder, the cut,
 * draining to idle, money conservation (dirty == clean + fees), and the Cut/deposit/
 * Dt edge cases. Prefix GTC.Systems.Economy.Laundering.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace MoneyLaunderingTest
{
    FMoneyLaundering::FParams Tuning()
    {
        FMoneyLaundering::FParams P; // cut 0.15, 1000/hr, capacity 50000
        return P;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMoneyLaunderingTest,
    "GTC.Systems.Economy.Laundering",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMoneyLaunderingTest::RunTest(const FString& Parameters)
{
    using namespace MoneyLaunderingTest;

    // ---- Fresh front is empty and idle ----------------------------------------
    {
        FMoneyLaundering L;
        L.Configure(Tuning());
        TestTrue(TEXT("no dirty cash"), FMath::Abs(L.DirtyBalance()) <= Eps);
        TestTrue(TEXT("idle"), L.IsIdle());
        TestTrue(TEXT("full capacity free"), FMath::IsNearlyEqual(L.SpaceRemaining(), 50000.0, Eps));
        TestTrue(TEXT("advancing an empty front pays nothing"), FMath::Abs(L.Advance(10.0)) <= Eps);
    }

    // ---- Deposit respects the capacity ceiling --------------------------------
    {
        FMoneyLaundering L;
        L.Configure(Tuning());
        TestTrue(TEXT("first deposit fully accepted"), FMath::IsNearlyEqual(L.Deposit(10000.0), 10000.0, Eps));
        TestTrue(TEXT("dirty pool is 10000"), FMath::IsNearlyEqual(L.DirtyBalance(), 10000.0, Eps));
        TestFalse(TEXT("no longer idle"), L.IsIdle());

        // Only 40000 more fits.
        TestTrue(TEXT("over-capacity deposit is clamped to the space left"),
            FMath::IsNearlyEqual(L.Deposit(50000.0), 40000.0, Eps));
        TestTrue(TEXT("pool is now at capacity"), FMath::IsNearlyEqual(L.DirtyBalance(), 50000.0, Eps));
        TestTrue(TEXT("a full front takes nothing"), FMath::Abs(L.Deposit(1000.0)) <= Eps);
        TestTrue(TEXT("no space left"), FMath::Abs(L.SpaceRemaining()) <= Eps);
    }

    // ---- Laundering is throughput-limited and takes the cut --------------------
    {
        FMoneyLaundering L;
        L.Configure(Tuning());
        L.Deposit(50000.0);

        const double H1 = L.Advance(1.0); // launders 1000 -> clean 850, fee 150
        TestTrue(TEXT("1h washes 1000 -> 850 clean"), FMath::IsNearlyEqual(H1, 850.0, Eps));
        TestTrue(TEXT("dirty drops by 1000"), FMath::IsNearlyEqual(L.DirtyBalance(), 49000.0, Eps));
        TestTrue(TEXT("fee of 150 skimmed"), FMath::IsNearlyEqual(L.TotalFees(), 150.0, Eps));

        const double Half = L.Advance(0.5); // launders 500 -> clean 425
        TestTrue(TEXT("half an hour washes 500 -> 425"), FMath::IsNearlyEqual(Half, 425.0, Eps));
    }

    // ---- A long run drains the pool to idle -----------------------------------
    {
        FMoneyLaundering L;
        L.Configure(Tuning());
        L.Deposit(50000.0);
        const double Big = L.Advance(1000.0); // far more than enough throughput
        TestTrue(TEXT("drains the whole pool"), L.IsIdle());
        TestTrue(TEXT("pool empty"), FMath::Abs(L.DirtyBalance()) <= Eps);
        // Clean over the whole drain = 50000 * 0.85 = 42500.
        TestTrue(TEXT("clean from a full drain is 42500"), FMath::IsNearlyEqual(Big, 42500.0, Eps));
    }

    // ---- Money is conserved: dirty in == clean out + fees ---------------------
    {
        FMoneyLaundering L;
        L.Configure(Tuning());
        const double In = L.Deposit(10000.0);
        double Clean = 0.0;
        for (int32 I = 0; I < 20; ++I)
        {
            Clean += L.Advance(1.0); // 20h * 1000/hr drains 10000
        }
        TestTrue(TEXT("fully drained"), L.IsIdle());
        TestTrue(TEXT("clean + fees == dirty deposited"),
            FMath::IsNearlyEqual(Clean + L.TotalFees(), In, Eps));
        TestTrue(TEXT("clean is 85% of the deposit"), FMath::IsNearlyEqual(Clean, 8500.0, Eps));
    }

    // ---- Cut extremes ---------------------------------------------------------
    {
        FMoneyLaundering Free;
        FMoneyLaundering::FParams NoCut = Tuning();
        NoCut.Cut = 0.0;
        Free.Configure(NoCut);
        Free.Deposit(1000.0);
        TestTrue(TEXT("zero cut washes 1:1"), FMath::IsNearlyEqual(Free.Advance(1.0), 1000.0, Eps));

        FMoneyLaundering AllFee;
        FMoneyLaundering::FParams FullCut = Tuning();
        FullCut.Cut = 1.0;
        AllFee.Configure(FullCut);
        AllFee.Deposit(1000.0);
        TestTrue(TEXT("full cut pays nothing clean"), FMath::Abs(AllFee.Advance(1.0)) <= Eps);
        TestTrue(TEXT("...but still drains the dirty pool"), FMath::IsNearlyEqual(AllFee.DirtyBalance(), 0.0, Eps));
    }

    // ---- Negative deposit / Dt are no-ops -------------------------------------
    {
        FMoneyLaundering L;
        L.Configure(Tuning());
        L.Deposit(5000.0);
        TestTrue(TEXT("negative deposit takes nothing"), FMath::Abs(L.Deposit(-100.0)) <= Eps);
        const double Before = L.DirtyBalance();
        TestTrue(TEXT("negative Dt launders nothing"), FMath::Abs(L.Advance(-5.0)) <= Eps);
        TestTrue(TEXT("dirty pool unchanged by negative Dt"), FMath::IsNearlyEqual(L.DirtyBalance(), Before, Eps));
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
