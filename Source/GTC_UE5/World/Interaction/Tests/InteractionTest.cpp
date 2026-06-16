// Copyright Epic Games, Inc. All Rights Reserved.

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "../Interaction.h"

// Parity oracle: game/tests/unit/test_interaction.gd. Each It(...) maps 1:1 to one
// Godot test function. The selection is integer index identity, so no float
// tolerance is needed; distances use FVector::Dist exactly as Godot
// Vector3.distance_to. Positions stay in the Godot XZ frame (no Z-up remap).
BEGIN_DEFINE_SPEC(FInteractionSpec, "GTC.World.Interaction",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FInteractionSpec)

void FInteractionSpec::Define()
{
    // test_no_points_gives_none
    It("returns None when there are no points", [this]()
    {
        const TArray<FVector> Points;
        TestEqual(TEXT("empty candidate set selects nothing"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 3.0), FInteraction::None);
    });

    // test_point_out_of_reach_is_ignored
    It("ignores a point out of reach", [this]()
    {
        const TArray<FVector> Points{ FVector(10, 0, 0) };
        TestEqual(TEXT("point beyond reach selects nothing"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 3.0), FInteraction::None);
    });

    // test_point_within_reach_is_selected
    It("selects a point within reach", [this]()
    {
        const TArray<FVector> Points{ FVector(2, 0, 0) };
        TestEqual(TEXT("point inside reach is selected"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 3.0), 0);
    });

    // test_nearest_of_several_wins
    It("picks the nearest of several", [this]()
    {
        const TArray<FVector> Points{ FVector(2.5, 0, 0), FVector(1.0, 0, 0), FVector(2.0, 0, 0) };
        TestEqual(TEXT("closest candidate index wins"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 3.0), 1);
    });

    // test_reach_boundary_is_inclusive
    It("treats the reach boundary as inclusive", [this]()
    {
        const TArray<FVector> Points{ FVector(3, 0, 0) };
        TestEqual(TEXT("point exactly at reach is selected"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 3.0), 0);
    });

    // test_ties_resolve_to_lower_index
    It("resolves ties to the lower index", [this]()
    {
        const TArray<FVector> Points{ FVector(2, 0, 0), FVector(0, 0, 2) };
        TestEqual(TEXT("equal distances keep the first index"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 3.0), 0);
    });

    // test_non_positive_reach_selects_nothing
    It("selects nothing for a non-positive reach", [this]()
    {
        const TArray<FVector> Points{ FVector(0, 0, 0) };
        TestEqual(TEXT("zero reach selects nothing"),
            FInteraction::Nearest(Points, FVector::ZeroVector, 0.0), FInteraction::None);
    });
}

#endif // WITH_AUTOMATION_TESTS
