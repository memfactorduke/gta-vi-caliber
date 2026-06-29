// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../BribeOffer.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FBribeOffer — bribing a cop to drop heat. GTC-original (no Godot oracle).
 * Pins the per-star quote, the heat ceiling (too hot to bribe / no heat to bribe),
 * affordability, the star reduction, the greed escalation, the cooldown reset, a
 * bribe restarting the cooldown, and negative-Dt clamping. Prefix
 * GTC.Systems.Wanted.Bribe. All inputs are method calls, so no shared helpers.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FBribeOfferTest,
    "GTC.Systems.Wanted.Bribe",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FBribeOfferTest::RunTest(const FString& Parameters)
{
    const FBribeOffer::FParams P; // max 3 stars, 250/star, greed 0.5, cooldown 6h, -1 star

    // ---- Quoting: per star, nothing when not bribable --------------------------
    {
        FBribeOffer B;
        B.Configure(P);
        TestEqual(TEXT("no recent bribes"), B.RecentBribes(), 0);
        TestTrue(TEXT("2 stars quotes 500"), FMath::IsNearlyEqual(B.QuoteCost(2), 500.0, Eps));
        TestTrue(TEXT("3 stars quotes 750"), FMath::IsNearlyEqual(B.QuoteCost(3), 750.0, Eps));
        TestTrue(TEXT("no heat -> no quote"), FMath::Abs(B.QuoteCost(0)) <= Eps);
        TestTrue(TEXT("too hot (4) -> no quote"), FMath::Abs(B.QuoteCost(4)) <= Eps);
    }

    // ---- Eligibility + affordability ------------------------------------------
    {
        FBribeOffer B;
        B.Configure(P);
        TestTrue(TEXT("can bribe 2 stars with 500"), B.CanBribe(2, 500.0));
        TestFalse(TEXT("can't bribe 2 stars with 499"), B.CanBribe(2, 499.0));
        TestFalse(TEXT("can't bribe with no heat"), B.CanBribe(0, 9999.0));
        TestFalse(TEXT("can't bribe when too hot"), B.CanBribe(4, 9999.0));
    }

    // ---- A successful bribe drops a star and stokes greed ----------------------
    {
        FBribeOffer B;
        B.Configure(P);
        const FBribeOffer::FResult R = B.Bribe(2, 1000.0);
        TestTrue(TEXT("bribe accepted"), R.bAccepted);
        TestTrue(TEXT("cost 500"), FMath::IsNearlyEqual(R.Cost, 500.0, Eps));
        TestEqual(TEXT("dropped to 1 star"), R.StarsAfter, 1);
        TestEqual(TEXT("one bribe on the books"), B.RecentBribes(), 1);

        // Greed: the same bribe now costs more.
        TestTrue(TEXT("greed raises the next quote to 750"), FMath::IsNearlyEqual(B.QuoteCost(2), 750.0, Eps));
        const FBribeOffer::FResult R2 = B.Bribe(2, 1000.0);
        TestTrue(TEXT("second bribe accepted at 750"), R2.bAccepted && FMath::IsNearlyEqual(R2.Cost, 750.0, Eps));
        TestTrue(TEXT("third quote climbs to 1000"), FMath::IsNearlyEqual(B.QuoteCost(2), 1000.0, Eps));
    }

    // ---- Declined bribes change nothing ---------------------------------------
    {
        FBribeOffer B;
        B.Configure(P);
        const FBribeOffer::FResult Hot = B.Bribe(4, 99999.0); // too hot
        TestFalse(TEXT("too-hot bribe declined"), Hot.bAccepted);
        TestEqual(TEXT("stars unchanged when too hot"), Hot.StarsAfter, 4);
        TestEqual(TEXT("no greed from a declined bribe"), B.RecentBribes(), 0);

        const FBribeOffer::FResult Broke = B.Bribe(2, 100.0); // can't afford
        TestFalse(TEXT("unaffordable bribe declined"), Broke.bAccepted);
        TestTrue(TEXT("quote still reported on a decline"), FMath::IsNearlyEqual(Broke.Cost, 500.0, Eps));
        TestEqual(TEXT("stars unchanged when broke"), Broke.StarsAfter, 2);
    }

    // ---- Going straight long enough resets the greed price --------------------
    {
        FBribeOffer B;
        B.Configure(P);
        B.Bribe(2, 1000.0);
        B.Bribe(2, 1000.0); // greed = 2 bribes -> quote 1000
        TestTrue(TEXT("price elevated by greed"), FMath::IsNearlyEqual(B.QuoteCost(2), 1000.0, Eps));

        B.Advance(3.0); // partial cooldown
        TestEqual(TEXT("partial cooldown keeps greed"), B.RecentBribes(), 2);
        B.Advance(3.0); // total 6h >= cooldown
        TestEqual(TEXT("full cooldown forgets greed"), B.RecentBribes(), 0);
        TestTrue(TEXT("price back to base"), FMath::IsNearlyEqual(B.QuoteCost(2), 500.0, Eps));
    }

    // ---- A bribe restarts the cooldown clock ----------------------------------
    {
        FBribeOffer B;
        B.Configure(P);
        B.Bribe(2, 1000.0);  // greed 1
        B.Advance(5.0);      // almost cooled
        B.Bribe(2, 1000.0);  // restarts the clock, greed 2
        B.Advance(5.0);      // only 5h since the last bribe (< 6) -> still greedy
        TestEqual(TEXT("the second bribe restarted the cooldown"), B.RecentBribes(), 2);
    }

    // ---- Star reduction floors at 0; negative Dt is a no-op --------------------
    {
        FBribeOffer B;
        B.Configure(P);
        const FBribeOffer::FResult One = B.Bribe(1, 1000.0); // 1 - 1 = 0
        TestTrue(TEXT("one-star bribe accepted"), One.bAccepted);
        TestEqual(TEXT("dropped to 0 stars"), One.StarsAfter, 0);

        B.Advance(-10.0); // ignored
        TestEqual(TEXT("negative Dt doesn't reset greed"), B.RecentBribes(), 1);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
