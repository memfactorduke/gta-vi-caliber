// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../CombatAi.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Parity oracle: game/tests/unit/test_combat_ai.gd. Each It(...) maps 1:1 to one
// Godot test_* function, asserting the SAME conditions as the oracle. Godot
// is_equal_approx is rendered with the shared GtcTest::Eps tolerance.
BEGIN_DEFINE_SPEC(FCombatAiSpec, "GTC.AI.Combat.CombatAi",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
    // Mirrors the oracle's _band() helper: engagement_band(14.0, 0.25).
    static FVector2D Band() { return FCombatAi::EngagementBand(14.0, 0.25); }
    static const FVector Origin;
    static const FVector Far;
END_DEFINE_SPEC(FCombatAiSpec)

const FVector FCombatAiSpec::Origin = FVector::ZeroVector;
const FVector FCombatAiSpec::Far = FVector(50, 0, 0);

void FCombatAiSpec::Define()
{
    // --- engagement_band ---------------------------------------------------

    // test_engagement_band
    It("brackets the preferred range and never inverts on wide hysteresis", [this]()
    {
        const FVector2D B = FCombatAi::EngagementBand(14.0, 0.25);
        TestTrue(TEXT("band brackets 14"), B.X < 14.0 && B.Y > 14.0);
        const FVector2D Wide = FCombatAi::EngagementBand(10.0, 5.0);
        TestTrue(TEXT("wide hysteresis stays ordered"), Wide.X >= 0.0 && Wide.X < Wide.Y);
    });

    // --- in_firing_arc -----------------------------------------------------

    // test_firing_arc_front_vs_back
    It("fires within the arc but not behind", [this]()
    {
        const bool bAhead = FCombatAi::InFiringArc(
            FVector(1, 0, 0), FVector(1, 0, 0), FCombatAi::DefaultArcHalf);
        const bool bBehind = FCombatAi::InFiringArc(
            FVector(1, 0, 0), FVector(-1, 0, 0), FCombatAi::DefaultArcHalf);
        TestTrue(TEXT("ahead in arc, behind not"), bAhead && !bBehind);
    });

    // test_firing_arc_ignores_height
    It("ignores height in the firing arc", [this]()
    {
        TestTrue(TEXT("planar target with Y offset still in arc"),
            FCombatAi::InFiringArc(FVector(1, 0, 0), FVector(1, 9, 0), FCombatAi::DefaultArcHalf));
    });

    // test_firing_arc_false_on_zero_vector
    It("returns false on a zero facing vector", [this]()
    {
        TestFalse(TEXT("zero facing is never in arc"),
            FCombatAi::InFiringArc(FVector::ZeroVector, FVector(1, 0, 0), FCombatAi::DefaultArcHalf));
    });

    // --- decide_action -----------------------------------------------------

    // test_advance_when_out_of_range
    It("advances when out of range", [this]()
    {
        TestTrue(TEXT("advance"),
            FCombatAi::DecideAction(40.0, Band(), true, true, 1.0, 0.8, 30) == ECombatAction::Advance);
    });

    // test_engage_when_in_band_and_aimed
    It("engages when in band and aimed", [this]()
    {
        TestTrue(TEXT("engage"),
            FCombatAi::DecideAction(14.0, Band(), true, true, 1.0, 0.8, 30) == ECombatAction::Engage);
    });

    // test_reposition_when_in_band_but_not_aimed
    It("repositions when in band but not aimed", [this]()
    {
        TestTrue(TEXT("reposition"),
            FCombatAi::DecideAction(14.0, Band(), true, false, 1.0, 0.8, 30) == ECombatAction::Reposition);
    });

    // test_reposition_when_too_close
    It("repositions when too close", [this]()
    {
        TestTrue(TEXT("reposition"),
            FCombatAi::DecideAction(5.0, Band(), true, true, 1.0, 0.8, 30) == ECombatAction::Reposition);
    });

    // test_advance_when_no_line_of_sight
    It("advances when no line of sight", [this]()
    {
        TestTrue(TEXT("advance over firing blind"),
            FCombatAi::DecideAction(14.0, Band(), false, true, 1.0, 0.8, 30) == ECombatAction::Advance);
    });

    // test_take_cover_when_hurt_but_lethal_presses_on
    It("takes cover when hurt but lethal presses on", [this]()
    {
        const ECombatAction Hurt = FCombatAi::DecideAction(14.0, Band(), true, true, 0.3, 0.6, 30);
        const ECombatAction Lethal = FCombatAi::DecideAction(14.0, Band(), true, true, 0.3, 1.0, 30);
        TestTrue(TEXT("hurt covers, lethal engages"),
            Hurt == ECombatAction::TakeCover && Lethal == ECombatAction::Engage);
    });

    // test_retreat_when_badly_hurt_and_timid
    It("retreats when badly hurt and timid", [this]()
    {
        TestTrue(TEXT("retreat"),
            FCombatAi::DecideAction(14.0, Band(), true, true, 0.1, 0.3, 30) == ECombatAction::Retreat);
    });

    // test_out_of_ammo_branches_on_resolve
    It("branches on resolve when out of ammo", [this]()
    {
        const ECombatAction Timid = FCombatAi::DecideAction(14.0, Band(), true, true, 1.0, 0.4, 0);
        const ECombatAction Lethal = FCombatAi::DecideAction(14.0, Band(), true, true, 1.0, 0.9, 0);
        TestTrue(TEXT("timid retreats, lethal repositions"),
            Timid == ECombatAction::Retreat && Lethal == ECombatAction::Reposition);
    });

    // --- should_fire -------------------------------------------------------

    // test_fires_only_when_engaging_and_ready
    It("fires only when engaging and ready", [this]()
    {
        TestTrue(TEXT("engage + ready fires"), FCombatAi::ShouldFire(ECombatAction::Engage, true));
        TestFalse(TEXT("engage + not ready holds"), FCombatAi::ShouldFire(ECombatAction::Engage, false));
        TestFalse(TEXT("reposition never fires"), FCombatAi::ShouldFire(ECombatAction::Reposition, true));
    });

    // --- fire_interval -----------------------------------------------------

    // test_fire_interval_scales_and_clamps
    It("scales and clamps the fire interval", [this]()
    {
        TestTrue(TEXT("higher aggression -> shorter interval"),
            FCombatAi::FireInterval(1.0, 1.0) < FCombatAi::FireInterval(1.0, 0.0));
        const double Lo = FCombatAi::FireInterval(1.0, 5.0);
        const double Hi = FCombatAi::FireInterval(1.0, -5.0);
        TestTrue(TEXT("clamps to bounds"),
            FMath::IsNearlyEqual(Lo, 0.6, Eps) && FMath::IsNearlyEqual(Hi, 1.8, Eps));
    });

    // --- desired_move ------------------------------------------------------

    // test_advance_and_retreat_are_radial
    It("makes advance and retreat radial", [this]()
    {
        const FVector Adv = FCombatAi::DesiredMove(ECombatAction::Advance, Origin, Far, 1.0);
        const FVector Ret = FCombatAi::DesiredMove(ECombatAction::Retreat, Origin, Far, 1.0);
        TestTrue(TEXT("advance toward, retreat away"),
            Adv.Dot(FVector(1, 0, 0)) > 0.99 && Ret.Dot(FVector(1, 0, 0)) < -0.99);
    });

    // test_engage_holds_position
    It("holds position on engage", [this]()
    {
        TestTrue(TEXT("engage move is zero"),
            FCombatAi::DesiredMove(ECombatAction::Engage, Origin, Far, 1.0) == FVector::ZeroVector);
    });

    // test_reposition_is_lateral_and_normalized
    It("makes reposition lateral and normalized", [this]()
    {
        const FVector Dir = FCombatAi::DesiredMove(ECombatAction::Reposition, Origin, Far, 1.0);
        TestTrue(TEXT("lateral dominates radial and is unit"),
            FMath::Abs(Dir.Z) > FMath::Abs(Dir.X) && Dir.IsNormalized());
    });

    // test_reposition_side_flips_with_sign
    It("flips reposition side with the sign", [this]()
    {
        const FVector Left = FCombatAi::DesiredMove(ECombatAction::Reposition, Origin, Far, 1.0);
        const FVector Right = FCombatAi::DesiredMove(ECombatAction::Reposition, Origin, Far, -1.0);
        TestTrue(TEXT("z sign flips"), FMath::Sign(Left.Z) != FMath::Sign(Right.Z));
    });

    // --- move_speed --------------------------------------------------------

    // test_engage_speed_is_zero
    It("makes engage speed zero", [this]()
    {
        TestTrue(TEXT("engage speed is zero"),
            FMath::IsNearlyEqual(FCombatAi::MoveSpeed(ECombatAction::Engage, 6.0), 0.0, Eps));
    });

    // test_advance_runs_and_outpaces_reposition
    It("runs on advance and outpaces reposition", [this]()
    {
        const double Adv = FCombatAi::MoveSpeed(ECombatAction::Advance, 6.0);
        TestTrue(TEXT("advance full speed, reposition slower"),
            FMath::IsNearlyEqual(Adv, 6.0, Eps) && FCombatAi::MoveSpeed(ECombatAction::Reposition, 6.0) < Adv);
    });
}

#endif // WITH_AUTOMATION_TESTS
