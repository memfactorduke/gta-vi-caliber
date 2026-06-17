// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PoliceEscalation.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Parity tests for FPoliceEscalation, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_police_escalation.gd. Tolerance mirrors the oracle's
 * is_equal_approx (Eps = 1e-4); sizes, enum equality, booleans and integer tiers
 * assert exactly. Grouped by oracle concern (prefix
 * GTC.AI.PoliceDispatch.PoliceEscalation).
 *
 * PURE-CORE boundary: a spawner reading ResponseUnits() to instance prefabs is
 * the deferred Wave-3 adapter and is NOT tested.
 */

// --- response composition ---------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceEscalationResponseTest,
    "GTC.AI.PoliceDispatch.PoliceEscalation.Response",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceEscalationResponseTest::RunTest(const FString& Parameters)
{
    // test_zero_stars_empty_response
    TestTrue(TEXT("zero stars empty response"),
        FPoliceEscalation::ResponseUnits(0).IsEmpty());
    // test_one_star_is_a_beat_cop
    {
        const TArray<EPoliceUnitType> Units = FPoliceEscalation::ResponseUnits(1);
        TestEqual(TEXT("one star size 1"), Units.Num(), 1);
        TestTrue(TEXT("one star is beat cop"), Units.Num() == 1 && Units[0] == EPoliceUnitType::BeatCop);
    }
    // test_response_never_shrinks_with_stars
    {
        bool bNeverShrinks = true;
        for (int32 S = 0; S < FPoliceEscalation::MaxStars; ++S)
        {
            if (FPoliceEscalation::ResponseUnits(S + 1).Num() < FPoliceEscalation::ResponseUnits(S).Num())
            {
                bNeverShrinks = false;
            }
        }
        TestTrue(TEXT("response never shrinks with stars"), bNeverShrinks);
    }
    // test_response_grows_across_band
    TestTrue(TEXT("response grows across band"),
        FPoliceEscalation::ResponseUnits(6).Num() > FPoliceEscalation::ResponseUnits(1).Num());
    // test_returned_array_is_a_copy
    {
        TArray<EPoliceUnitType> Units = FPoliceEscalation::ResponseUnits(3);
        Units.Add(static_cast<EPoliceUnitType>(999));
        TestEqual(TEXT("returned array is a copy"), FPoliceEscalation::ResponseUnits(3).Num(), 4);
    }

    return true;
}

// --- heavy asset thresholds -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceEscalationHeavyAssetsTest,
    "GTC.AI.PoliceDispatch.PoliceEscalation.HeavyAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceEscalationHeavyAssetsTest::RunTest(const FString& Parameters)
{
    // test_zero_stars_no_heavy_assets
    TestTrue(TEXT("zero stars no heavy assets"),
        !FPoliceEscalation::HasSwat(0) && !FPoliceEscalation::HasHelicopter(0)
        && !FPoliceEscalation::HasMilitary(0));
    // test_swat_threshold_flips_at_three
    TestTrue(TEXT("swat flips at three"),
        !FPoliceEscalation::HasSwat(2) && FPoliceEscalation::HasSwat(3));
    // test_swat_stays_true_above_threshold
    TestTrue(TEXT("swat stays true above threshold"),
        FPoliceEscalation::HasSwat(3) && FPoliceEscalation::HasSwat(4) && FPoliceEscalation::HasSwat(6));
    // test_helicopter_threshold_flips_at_four
    TestTrue(TEXT("helicopter flips at four"),
        !FPoliceEscalation::HasHelicopter(3) && FPoliceEscalation::HasHelicopter(4));
    // test_helicopter_stays_true_above_threshold
    TestTrue(TEXT("helicopter stays true above threshold"),
        FPoliceEscalation::HasHelicopter(4) && FPoliceEscalation::HasHelicopter(6));
    // test_military_only_at_six
    TestTrue(TEXT("military only at six"),
        !FPoliceEscalation::HasMilitary(5) && FPoliceEscalation::HasMilitary(6));

    return true;
}

// --- aggression / roadblock / interval --------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceEscalationCurvesTest,
    "GTC.AI.PoliceDispatch.PoliceEscalation.Curves",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceEscalationCurvesTest::RunTest(const FString& Parameters)
{
    // test_zero_stars_zero_aggression
    TestEqual(TEXT("zero stars zero aggression"), FPoliceEscalation::Aggression(0), 0.0, Eps);
    // test_zero_stars_no_roadblock
    TestEqual(TEXT("zero stars no roadblock"), FPoliceEscalation::RoadblockChance(0), 0.0, Eps);
    // test_aggression_monotonic_in_unit_range
    {
        bool bMonotonic = true;
        for (int32 S = 0; S < FPoliceEscalation::MaxStars; ++S)
        {
            const double Lo = FPoliceEscalation::Aggression(S);
            const double Hi = FPoliceEscalation::Aggression(S + 1);
            if (Hi < Lo || Lo < 0.0 || Hi > 1.0)
            {
                bMonotonic = false;
            }
        }
        TestTrue(TEXT("aggression monotonic in unit range"), bMonotonic);
    }
    // test_aggression_peaks_at_one
    TestEqual(TEXT("aggression peaks at one"), FPoliceEscalation::Aggression(6), 1.0, Eps);
    // test_roadblock_chance_monotonic_and_bounded
    {
        bool bMonotonic = true;
        for (int32 S = 0; S < FPoliceEscalation::MaxStars; ++S)
        {
            const double Lo = FPoliceEscalation::RoadblockChance(S);
            const double Hi = FPoliceEscalation::RoadblockChance(S + 1);
            if (Hi < Lo || Lo < 0.0 || Hi > 1.0)
            {
                bMonotonic = false;
            }
        }
        TestTrue(TEXT("roadblock chance monotonic and bounded"), bMonotonic);
    }
    // test_reinforcement_interval_non_increasing_and_positive
    {
        bool bOk = true;
        for (int32 S = 0; S < FPoliceEscalation::MaxStars; ++S)
        {
            const double Lo = FPoliceEscalation::ReinforcementInterval(S);
            const double Hi = FPoliceEscalation::ReinforcementInterval(S + 1);
            if (Hi > Lo || Hi <= 0.0)
            {
                bOk = false;
            }
        }
        TestTrue(TEXT("reinforcement interval non-increasing and positive"), bOk);
    }

    return true;
}

// --- weapon tier ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceEscalationWeaponTierTest,
    "GTC.AI.PoliceDispatch.PoliceEscalation.WeaponTier",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceEscalationWeaponTierTest::RunTest(const FString& Parameters)
{
    // test_zero_stars_weapon_tier_zero
    TestEqual(TEXT("zero stars weapon tier zero"), FPoliceEscalation::WeaponTier(0), 0);
    // test_weapon_tier_non_decreasing
    {
        bool bNonDecreasing = true;
        for (int32 S = 0; S < FPoliceEscalation::MaxStars; ++S)
        {
            if (FPoliceEscalation::WeaponTier(S + 1) < FPoliceEscalation::WeaponTier(S))
            {
                bNonDecreasing = false;
            }
        }
        TestTrue(TEXT("weapon tier non-decreasing"), bNonDecreasing);
    }
    // test_weapon_tier_tops_out_at_military
    TestTrue(TEXT("weapon tier tops out at military"),
        FPoliceEscalation::WeaponTier(6) == 4 && FPoliceEscalation::WeaponTier(0) == 0);

    return true;
}

// --- clamping ---------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoliceEscalationClampTest,
    "GTC.AI.PoliceDispatch.PoliceEscalation.Clamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoliceEscalationClampTest::RunTest(const FString& Parameters)
{
    // test_high_stars_clamp_to_six
    TestTrue(TEXT("high stars clamp to six"),
        FPoliceEscalation::ResponseUnits(7).Num() == FPoliceEscalation::ResponseUnits(6).Num()
        && FPoliceEscalation::HasMilitary(7));
    TestEqual(TEXT("aggression clamps high"),
        FPoliceEscalation::Aggression(7), FPoliceEscalation::Aggression(6), Eps);
    TestEqual(TEXT("weapon tier clamps high"),
        FPoliceEscalation::WeaponTier(7), FPoliceEscalation::WeaponTier(6));
    // test_negative_stars_clamp_to_zero
    TestTrue(TEXT("negative stars clamp to zero"),
        FPoliceEscalation::ResponseUnits(-1).IsEmpty() && !FPoliceEscalation::HasSwat(-1)
        && FPoliceEscalation::WeaponTier(-1) == 0);
    TestEqual(TEXT("aggression clamps negative"), FPoliceEscalation::Aggression(-1), 0.0, Eps);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
