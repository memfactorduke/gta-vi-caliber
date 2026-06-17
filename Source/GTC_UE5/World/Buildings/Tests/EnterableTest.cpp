// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Enterable.h"

// Parity oracle: game/tests/unit/test_enterable.gd. Each It(...) maps 1:1 to one
// the reference test function, asserting the SAME conditions as the oracle, including its
// 0.001 door-midpoint tolerance.
BEGIN_DEFINE_SPEC(FEnterableSpec, "GTC.World.Buildings.Enterable",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FEnterableSpec)

void FEnterableSpec::Define()
{
    // test_named_building_is_enterable
    It("treats a named building as enterable", [this]()
    {
        TestTrue(TEXT("named building is enterable"),
            FEnterable::IsEnterable(FBuilding(TEXT("U.S. Bank Tower"), TEXT("yes"))));
    });

    // test_public_type_is_enterable
    It("treats a public type as enterable", [this]()
    {
        TestTrue(TEXT("retail is enterable"),
            FEnterable::IsEnterable(FBuilding(TEXT(""), TEXT("retail"))));
    });

    // test_plain_house_is_not_enterable
    It("treats a plain house as not enterable", [this]()
    {
        TestFalse(TEXT("unnamed house is not enterable"),
            FEnterable::IsEnterable(FBuilding(TEXT(""), TEXT("house"))));
    });

    // test_door_is_midpoint_of_longest_edge
    It("places the door at the midpoint of the longest edge", [this]()
    {
        // A 10x2 rectangle: longest edges are the length-10 sides. Door on one of them.
        const TArray<FVector2D> Footprint = {
            FVector2D(0.0, 0.0),
            FVector2D(10.0, 0.0),
            FVector2D(10.0, 2.0),
            FVector2D(0.0, 2.0),
        };
        const FVector2D Door = FEnterable::DoorPoint(Footprint);
        const bool bOkX = FMath::Abs(Door.X - 5.0) < 0.001;
        const bool bOkY = FMath::Abs(Door.Y) < 0.001 || FMath::Abs(Door.Y - 2.0) < 0.001;
        TestTrue(TEXT("door at x=5 on one length-10 edge"), bOkX && bOkY);
    });

    // test_pick_caps_count
    It("caps the picked count", [this]()
    {
        TArray<FBuilding> Buildings;
        for (int32 I = 0; I < 10; ++I)
        {
            Buildings.Add(FBuilding(FString::Printf(TEXT("Shop %d"), I), TEXT("retail")));
        }
        TestEqual(TEXT("capped at 3"), FEnterable::Pick(Buildings, 3).Num(), 3);
    });

    // test_pick_skips_non_enterable
    It("skips non-enterable buildings", [this]()
    {
        const TArray<FBuilding> Buildings = {
            FBuilding(TEXT(""), TEXT("house")),
            FBuilding(TEXT("Cafe"), TEXT("retail")),
            FBuilding(TEXT(""), TEXT("yes")),
        };
        TestEqual(TEXT("only the one enterable is picked"),
            FEnterable::Pick(Buildings, 10).Num(), 1);
    });
}

#endif // WITH_AUTOMATION_TESTS
