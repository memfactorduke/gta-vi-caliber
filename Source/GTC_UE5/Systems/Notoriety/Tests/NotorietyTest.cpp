// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Notoriety.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Unit tests for FNotoriety — infamy accrual/decay, tiers, bounty, and hunter waves.
 * Prefix GTC.Systems.Notoriety.
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNotorietyTest,
    "GTC.Systems.Notoriety",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNotorietyTest::RunTest(const FString& Parameters)
{
    // --- Accrual + decay ---
    {
        FNotoriety N;
        TestTrue(TEXT("starts anonymous"), N.Tier() == ENotorietyTier::Anonymous);
        N.Add(FNotoriety::ForKill);
        TestTrue(TEXT("a kill adds infamy"), N.Notoriety > 0.0);
        N.Add(-100.0);
        TestTrue(TEXT("negative adds are ignored"),
            FMath::IsNearlyEqual(N.Notoriety, FNotoriety::ForKill, GtcTest::Eps));

        N.Notoriety = 10.0;
        N.Decay(4.0); // 4s * 0.5/s = 2
        TestTrue(TEXT("infamy bleeds over time"),
            FMath::IsNearlyEqual(N.Notoriety, 8.0, GtcTest::Eps));
        N.Decay(1000.0);
        TestTrue(TEXT("infamy floors at zero"), FMath::IsNearlyEqual(N.Notoriety, 0.0, GtcTest::Eps));
    }

    // --- Tiers ---
    {
        FNotoriety N;
        N.Notoriety = FNotoriety::KnownAt;
        TestTrue(TEXT("reaches Known"), N.Tier() == ENotorietyTier::Known);
        N.Notoriety = FNotoriety::NotoriousAt;
        TestTrue(TEXT("reaches Notorious"), N.Tier() == ENotorietyTier::Notorious);
        N.Notoriety = FNotoriety::LegendaryAt;
        TestTrue(TEXT("reaches Legendary"), N.Tier() == ENotorietyTier::Legendary);
    }

    // --- Bounty ---
    {
        FNotoriety N;
        N.Notoriety = FNotoriety::KnownAt - 1.0;
        TestEqual(TEXT("a nobody has no bounty"), N.BountyAmount(), 0.0);
        N.Notoriety = FNotoriety::KnownAt + 10.0;
        const double B1 = N.BountyAmount();
        TestTrue(TEXT("bounty appears past Known"), B1 > 0.0);
        N.Notoriety = FNotoriety::KnownAt + 100.0;
        TestTrue(TEXT("bounty grows with infamy"), N.BountyAmount() > B1);
    }

    // --- Hunter waves ---
    {
        FNotoriety N;
        TestFalse(TEXT("no hunters for a nobody"), N.HunterReady());
        TestEqual(TEXT("a nobody sends no hunters"), N.WaveSize(), 0);

        N.Notoriety = FNotoriety::NotoriousAt;
        TestTrue(TEXT("a notorious player draws hunters"), N.HunterReady());
        TestEqual(TEXT("notorious wave size is 2"), N.WaveSize(), 2);

        N.TriggerWave();
        TestFalse(TEXT("on cooldown right after a wave"), N.HunterReady());
        N.Decay(FNotoriety::HunterCooldownSec + 1.0);
        // Decay also bled some notoriety; top it back up to keep the tier for the readiness check.
        N.Notoriety = FNotoriety::NotoriousAt;
        TestTrue(TEXT("ready again after the cooldown"), N.HunterReady());

        N.Notoriety = FNotoriety::LegendaryAt;
        TestEqual(TEXT("legendary wave size is 4"), N.WaveSize(), 4);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
