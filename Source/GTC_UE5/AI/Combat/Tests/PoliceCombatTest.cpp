// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PoliceCombat.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// Parity oracle: game/tests/unit/test_police_combat.gd. Each It(...) maps 1:1 to
// one Godot test_* function, asserting the SAME conditions on the pure
// PoliceCombat + CombatAi + PoliceResponse composition.
BEGIN_DEFINE_SPEC(FPoliceCombatSpec, "GTC.AI.Combat.PoliceCombat",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
    static const FVector Fwd;
    static const FVector Back;
END_DEFINE_SPEC(FPoliceCombatSpec)

const FVector FPoliceCombatSpec::Fwd = FVector(1, 0, 0);
const FVector FPoliceCombatSpec::Back = FVector(-1, 0, 0);

void FPoliceCombatSpec::Define()
{
    // --- plan: positioning vs firing ---------------------------------------

    // test_far_target_advances_without_firing
    It("advances on a far target without firing", [this]()
    {
        const FPoliceCombat::FCombatPlan P = FPoliceCombat::Plan(40.0, true, Fwd, Fwd, 1.0, 3, 12, true);
        TestTrue(TEXT("advance, no fire"), P.Action == ECombatAction::Advance && !P.bFire);
    });

    // test_in_band_aimed_engages_and_fires
    It("engages and fires when in band and aimed", [this]()
    {
        const FPoliceCombat::FCombatPlan P = FPoliceCombat::Plan(16.0, true, Fwd, Fwd, 1.0, 3, 12, true);
        TestTrue(TEXT("engage, fire"), P.Action == ECombatAction::Engage && P.bFire);
    });

    // test_engage_holds_fire_until_cooldown_ready
    It("holds fire until the cooldown is ready", [this]()
    {
        const FPoliceCombat::FCombatPlan P = FPoliceCombat::Plan(16.0, true, Fwd, Fwd, 1.0, 3, 12, false);
        TestTrue(TEXT("engage, no fire"), P.Action == ECombatAction::Engage && !P.bFire);
    });

    // test_no_line_of_sight_advances_never_fires
    It("advances and never fires without line of sight", [this]()
    {
        const FPoliceCombat::FCombatPlan P = FPoliceCombat::Plan(16.0, false, Fwd, Fwd, 1.0, 3, 12, true);
        TestTrue(TEXT("advance, no fire"), P.Action == ECombatAction::Advance && !P.bFire);
    });

    // test_out_of_arc_repositions_without_firing
    It("repositions without firing when out of arc", [this]()
    {
        const FPoliceCombat::FCombatPlan P = FPoliceCombat::Plan(16.0, true, Fwd, Back, 1.0, 3, 12, true);
        TestTrue(TEXT("reposition, no fire, not in arc"),
            P.Action == ECombatAction::Reposition && !P.bFire && !P.bInArc);
    });

    // --- plan: survival & ammo ---------------------------------------------

    // test_hurt_takes_cover_unless_lethal_heat
    It("takes cover when hurt unless heat is lethal", [this]()
    {
        const FPoliceCombat::FCombatPlan Timid = FPoliceCombat::Plan(16.0, true, Fwd, Fwd, 0.3, 2, 12, true);
        const FPoliceCombat::FCombatPlan Lethal = FPoliceCombat::Plan(16.0, true, Fwd, Fwd, 0.3, 5, 12, true);
        TestTrue(TEXT("timid covers, lethal does not"),
            Timid.Action == ECombatAction::TakeCover && Lethal.Action != ECombatAction::TakeCover);
    });

    // test_out_of_ammo_branches_on_heat
    It("branches on heat when out of ammo", [this]()
    {
        const FPoliceCombat::FCombatPlan Lethal = FPoliceCombat::Plan(16.0, true, Fwd, Fwd, 1.0, 5, 0, true);
        const FPoliceCombat::FCombatPlan Timid = FPoliceCombat::Plan(16.0, true, Fwd, Fwd, 1.0, 1, 0, true);
        TestTrue(TEXT("lethal repositions, timid retreats"),
            Lethal.Action == ECombatAction::Reposition && Timid.Action == ECombatAction::Retreat);
    });

    // --- heat scaling ------------------------------------------------------

    // test_fire_cooldown_shrinks_with_heat
    It("shrinks the fire cooldown with heat", [this]()
    {
        const double Hot = FPoliceCombat::FireCooldown(5);
        const double Cold = FPoliceCombat::FireCooldown(1);
        TestTrue(TEXT("both positive, hot < cold"), Hot > 0.0 && Cold > 0.0 && Hot < Cold);
    });

    // test_chase_speed_grows_with_heat
    It("grows the chase speed with heat", [this]()
    {
        TestTrue(TEXT("5-star faster than 0-star"),
            FPoliceCombat::ChaseSpeed(7.0, 5) > FPoliceCombat::ChaseSpeed(7.0, 0));
    });

    // test_zero_stars_is_calm_and_slow
    It("is calm and slow at zero stars", [this]()
    {
        const FPoliceCombat::FCombatPlan P = FPoliceCombat::Plan(40.0, true, Fwd, Fwd, 1.0, 0, 12, true);
        TestTrue(TEXT("advance, no fire"), P.Action == ECombatAction::Advance && !P.bFire);
        const bool bSlowestFire = FPoliceCombat::FireCooldown(0) >= FPoliceCombat::FireCooldown(5);
        const bool bSlowestChase = FPoliceCombat::ChaseSpeed(7.0, 0) <= FPoliceCombat::ChaseSpeed(7.0, 5);
        TestTrue(TEXT("longest fire interval, slowest chase"), bSlowestFire && bSlowestChase);
    });

    // --- band sanity -------------------------------------------------------

    // test_band_brackets_preferred_range
    It("brackets the preferred range with the band", [this]()
    {
        const FVector2D B = FPoliceCombat::Band();
        TestTrue(TEXT("band brackets preferred range"),
            B.X < FPoliceCombat::PreferredRange && B.Y > FPoliceCombat::PreferredRange);
    });
}

#endif // WITH_AUTOMATION_TESTS
