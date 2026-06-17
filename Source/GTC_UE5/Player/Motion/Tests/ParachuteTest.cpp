// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../Parachute.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Parity tests for FParachute, mapped 1:1 from the Godot oracle
 * game/tests/unit/test_parachute.gd. is_equal_approx -> Eps=1e-4; exact state /
 * bool checks assert exactly. time_to_ground's Godot INF is asserted as true
 * +infinity (!FMath::IsFinite and > 0), matching the FGpsNavigation ETA pattern.
 */

using GtcTest::Eps;

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FParachuteTest,
    "GTC.Player.Motion.Parachute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FParachuteTest::RunTest(const FString& Parameters)
{
    // test_starts_in_freefall
    {
        FParachute P;
        TestTrue(TEXT("starts freefall"), P.State() == FParachute::EState::Freefall);
        TestFalse(TEXT("not deployed start"), P.IsDeployed());
    }

    // test_freefall_accelerates_under_gravity: 0 + 9.8*1 = 9.8
    {
        FParachute P(55.0, 6.0);
        TestEqual(TEXT("freefall +gravity"), P.UpdateFallSpeed(0.0, 1.0), 9.8, Eps);
    }

    // test_freefall_caps_at_terminal: 50 + 9.8 -> clamp 55
    {
        FParachute P(55.0, 6.0);
        TestEqual(TEXT("caps at terminal"), P.UpdateFallSpeed(50.0, 1.0), 55.0, Eps);
    }

    // test_freefall_never_exceeds_terminal_big_delta
    {
        FParachute P(55.0, 6.0);
        TestEqual(TEXT("never exceeds terminal"), P.UpdateFallSpeed(10.0, 100.0), 55.0, Eps);
    }

    // test_deploy_flips_state_once
    {
        FParachute P;
        const bool First = P.Deploy();
        const bool Second = P.Deploy();
        TestTrue(TEXT("first deploy true"), First);
        TestFalse(TEXT("second deploy false"), Second);
        TestTrue(TEXT("deployed after"), P.IsDeployed());
    }

    // test_deploy_only_from_freefall
    {
        FParachute P;
        P.Land();
        const bool Opened = P.Deploy();
        TestFalse(TEXT("no deploy after land"), Opened);
        TestTrue(TEXT("stays landed"), P.State() == FParachute::EState::Landed);
    }

    // test_deployed_decelerates_toward_canopy_rate: 50 - 12 = 38
    {
        FParachute P(55.0, 6.0);
        P.Deploy();
        TestEqual(TEXT("decel toward canopy"), P.UpdateFallSpeed(50.0, 1.0), 38.0, Eps);
    }

    // test_deployed_settles_at_canopy_rate: 8 - 12 -> floor 6
    {
        FParachute P(55.0, 6.0);
        P.Deploy();
        TestEqual(TEXT("settles at canopy"), P.UpdateFallSpeed(8.0, 1.0), 6.0, Eps);
    }

    // test_deployed_speeds_up_to_canopy_if_slow: 0 + 12 capped at 6
    {
        FParachute P(55.0, 6.0);
        P.Deploy();
        TestEqual(TEXT("speeds up to canopy"), P.UpdateFallSpeed(0.0, 1.0), 6.0, Eps);
    }

    // test_drift_larger_when_deployed_than_freefall
    {
        FParachute P;
        const FVector Steer(1.0, 0.0, 0.0);
        const FVector Open = P.HorizontalDrift(Steer, true, 10.0);
        const FVector Fall = P.HorizontalDrift(Steer, false, 10.0);
        TestTrue(TEXT("open drift larger"), Open.Size() > Fall.Size());
        TestTrue(TEXT("fall drift positive"), Fall.Size() > 0.0);
    }

    // test_drift_deployed_hits_glide_speed
    {
        FParachute P;
        const FVector Drift = P.HorizontalDrift(FVector(0.0, 0.0, 3.0), true, 10.0);
        TestEqual(TEXT("drift hits glide"), Drift.Size(), 10.0, Eps);
        TestEqual(TEXT("drift y zero"), Drift.Y, 0.0, Eps);
    }

    // test_drift_zero_with_no_input
    {
        FParachute P;
        const FVector Drift = P.HorizontalDrift(FVector::ZeroVector, true, 10.0);
        TestEqual(TEXT("no input drift x"), Drift.X, 0.0);
        TestEqual(TEXT("no input drift y"), Drift.Y, 0.0);
        TestEqual(TEXT("no input drift z"), Drift.Z, 0.0);
    }

    // test_drift_drops_vertical_input
    {
        FParachute P;
        const FVector Drift = P.HorizontalDrift(FVector(0.0, 9.0, 0.0), true, 10.0);
        TestEqual(TEXT("vertical drift x"), Drift.X, 0.0);
        TestEqual(TEXT("vertical drift y"), Drift.Y, 0.0);
        TestEqual(TEXT("vertical drift z"), Drift.Z, 0.0);
    }

    // test_landing_impact_zero_for_soft_landing
    {
        FParachute P;
        TestEqual(TEXT("soft landing zero"), P.LandingImpact(4.0, 6.0), 0.0, Eps);
    }

    // test_landing_impact_rises_past_safe_speed: (9-6)/6 = 0.5
    {
        FParachute P;
        TestEqual(TEXT("rises past safe"), P.LandingImpact(9.0, 6.0), 0.5, Eps);
    }

    // test_landing_impact_clamped_to_one
    {
        FParachute P;
        TestEqual(TEXT("impact clamp one"), P.LandingImpact(55.0, 6.0), 1.0, Eps);
    }

    // test_landing_impact_guards_zero_safe_speed
    {
        FParachute P;
        TestEqual(TEXT("zero safe guard"), P.LandingImpact(10.0, 0.0), 1.0, Eps);
    }

    // test_is_safe_landing_threshold
    {
        FParachute P;
        TestTrue(TEXT("safe at threshold"), P.IsSafeLanding(6.0, 6.0));
        TestFalse(TEXT("unsafe past threshold"), P.IsSafeLanding(6.1, 6.0));
    }

    // test_time_to_ground_alt_over_rate: 60/6 = 10
    {
        FParachute P;
        TestEqual(TEXT("time alt over rate"), P.TimeToGround(60.0, 6.0), 10.0, Eps);
    }

    // test_time_to_ground_guards_zero_rate: INF and 0 for neg altitude
    {
        FParachute P;
        const double Hover = P.TimeToGround(60.0, 0.0);
        TestFalse(TEXT("hover is infinite"), FMath::IsFinite(Hover));
        TestTrue(TEXT("hover infinity positive"), Hover > 0.0);
        TestEqual(TEXT("neg altitude landed"), P.TimeToGround(-5.0, 6.0), 0.0, Eps);
    }

    // test_land_and_reset
    {
        FParachute P;
        P.Deploy();
        P.Land();
        const bool Landed = P.State() == FParachute::EState::Landed;
        P.Reset();
        TestTrue(TEXT("was landed"), Landed);
        TestTrue(TEXT("reset to freefall"), P.State() == FParachute::EState::Freefall);
        TestFalse(TEXT("reset not deployed"), P.IsDeployed());
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
