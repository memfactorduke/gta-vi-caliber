// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../WantedLevel.h"

// Parity oracle: game/tests/unit/test_wanted_level.gd. Each It(...) maps 1:1 to
// one the reference test function, asserting the SAME conditions as the oracle with no
// added tolerances (the logic is pure integer/exact comparisons).
BEGIN_DEFINE_SPEC(FWantedLevelSpec, "GTC.World.Police.WantedLevel",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FWantedLevelSpec)

void FWantedLevelSpec::Define()
{
    // test_starts_unwanted
    It("starts unwanted", [this]()
    {
        FWantedLevel W;
        TestTrue(TEXT("zero stars and not wanted"), W.Stars() == 0 && !W.IsWanted());
    });

    // test_small_heat_is_one_star
    It("treats small heat as one star", [this]()
    {
        FWantedLevel W;
        W.AddHeat(1.0);
        TestTrue(TEXT("one star and wanted"), W.Stars() == 1 && W.IsWanted());
    });

    // test_heat_maps_to_higher_stars
    It("maps heat to higher stars", [this]()
    {
        FWantedLevel W;
        W.AddHeat(10.0);
        TestTrue(TEXT("four stars"), W.Stars() == 4);
    });

    // test_caps_at_five_stars
    It("caps at five stars", [this]()
    {
        FWantedLevel W;
        W.AddHeat(1000.0);
        TestTrue(TEXT("capped at MaxStars"), W.Stars() == FWantedLevel::MaxStars);
    });

    // test_crime_kinds_add_heat
    It("adds heat for crime kinds", [this]()
    {
        FWantedLevel W;
        W.AddCrime(TEXT("shooting"));
        TestTrue(TEXT("at least two stars"), W.Stars() >= 2);
    });

    // test_decay_reduces_stars
    It("reduces stars with decay", [this]()
    {
        FWantedLevel W;
        W.AddHeat(3.0); // 2 stars
        const int32 Before = W.Stars();
        for (int32 i = 0; i < 10; ++i)
        {
            W.Decay(1.0); // 10 s * 0.5 = 5 heat removed -> below 1 star
        }
        TestTrue(TEXT("was 2 stars, now 0"), Before == 2 && W.Stars() == 0);
    });

    // test_clear_resets
    It("resets on clear", [this]()
    {
        FWantedLevel W;
        W.AddHeat(12.0);
        W.Clear();
        TestTrue(TEXT("zero stars and not wanted"), W.Stars() == 0 && !W.IsWanted());
    });
}

#endif // WITH_AUTOMATION_TESTS
