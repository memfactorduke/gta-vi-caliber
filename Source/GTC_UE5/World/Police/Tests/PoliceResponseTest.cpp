// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../PoliceResponse.h"

// Parity oracle: game/tests/unit/test_police_response.gd. Each It(...) maps 1:1
// to one the reference test function, asserting the SAME conditions as the oracle with
// no added tolerances. The profile() dict-key-presence check is rendered as a
// typed field-equality assertion against the source functions (APPROVED).
BEGIN_DEFINE_SPEC(FPoliceResponseSpec, "GTC.World.Police.PoliceResponse",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FPoliceResponseSpec)

void FPoliceResponseSpec::Define()
{
    // test_no_police_at_zero_stars
    It("deploys no police at zero stars", [this]()
    {
        TestTrue(TEXT("no units and no helicopter"),
            FPoliceResponse::UnitsFor(0) == 0 && !FPoliceResponse::UsesHelicopter(0));
    });

    // test_units_escalate_with_stars
    It("escalates units with stars", [this]()
    {
        TestTrue(TEXT("units grow and top out at 8"),
            FPoliceResponse::UnitsFor(1) < FPoliceResponse::UnitsFor(3)
            && FPoliceResponse::UnitsFor(5) == 8);
    });

    // test_helicopter_joins_at_three_stars
    It("joins a helicopter at three stars", [this]()
    {
        TestTrue(TEXT("no heli at 2, heli at 3"),
            !FPoliceResponse::UsesHelicopter(2) && FPoliceResponse::UsesHelicopter(3));
    });

    // test_aggression_scales_zero_to_one
    It("scales aggression from zero to one", [this]()
    {
        TestTrue(TEXT("0 at 0 stars, 1 at 5 stars"),
            FPoliceResponse::Aggression(0) == 0.0 && FPoliceResponse::Aggression(5) == 1.0);
    });

    // test_spawn_radius_widens_with_heat
    It("widens spawn radius with heat", [this]()
    {
        TestTrue(TEXT("radius at 5 stars exceeds 1 star"),
            FPoliceResponse::SpawnRadius(5) > FPoliceResponse::SpawnRadius(1));
    });

    // test_profile_bundles_fields
    It("bundles the profile fields", [this]()
    {
        const FPoliceProfile P = FPoliceResponse::Profile(4);
        TestTrue(TEXT("each field matches its source function"),
            P.Units == FPoliceResponse::UnitsFor(4)
            && P.bHelicopter == FPoliceResponse::UsesHelicopter(4)
            && P.Aggression == FPoliceResponse::Aggression(4)
            && P.SpawnRadius == FPoliceResponse::SpawnRadius(4));
    });

    // test_star_count_is_clamped
    It("clamps the star count", [this]()
    {
        TestTrue(TEXT("99 -> 8 units, -3 -> 0 units"),
            FPoliceResponse::UnitsFor(99) == 8 && FPoliceResponse::UnitsFor(-3) == 0);
    });
}

#endif // WITH_AUTOMATION_TESTS
