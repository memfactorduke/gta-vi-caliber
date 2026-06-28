// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../CollectionTracker.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FCollectionTracker — the 100%-completion ledger. GTC-original (no Godot
 * oracle). Pins idempotent collection (dupes/out-of-range/unknown ignored), per-
 * category fractions, count-weighted overall completion across categories, the
 * reward tiers at 25/50/75/100%, zero-total categories, re-definition resetting a
 * category, and the empty tracker. Prefix GTC.Systems.Collection.
 *
 * Helpers live in a NAMED namespace so the unity build can't collide them with
 * another test's same-named anonymous-namespace helper.
 */
namespace CollectionTrackerTest
{
    const FString Tags = TEXT("spray_tags");
    const FString Packages = TEXT("hidden_packages");
    const FString Photos = TEXT("photo_viewpoints");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FCollectionTrackerTest,
    "GTC.Systems.Collection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FCollectionTrackerTest::RunTest(const FString& Parameters)
{
    using namespace CollectionTrackerTest;
    using EReward = FCollectionTracker::EReward;

    // ---- Empty tracker: nothing to collect reads as 100% ----------------------
    {
        FCollectionTracker T;
        TestEqual(TEXT("no categories"), T.CategoryCount(), 0);
        TestEqual(TEXT("empty grand total is 0"), T.GrandTotal(), 0);
        TestTrue(TEXT("empty completion is 100%"), FMath::IsNearlyEqual(T.Completion(), 1.0, Eps));
        TestTrue(TEXT("empty is complete"), T.IsComplete());
        TestTrue(TEXT("empty earns Platinum"), T.Reward() == EReward::Platinum);
    }

    // ---- A defined category starts empty --------------------------------------
    {
        FCollectionTracker T;
        T.DefineCategory(Tags, 4);
        TestEqual(TEXT("one category"), T.CategoryCount(), 1);
        TestEqual(TEXT("found 0"), T.FoundIn(Tags), 0);
        TestEqual(TEXT("total 4"), T.TotalIn(Tags), 4);
        TestTrue(TEXT("fraction 0"), FMath::Abs(T.FractionIn(Tags)) <= Eps);
        TestFalse(TEXT("not complete"), T.IsCategoryComplete(Tags));
        TestEqual(TEXT("grand total 4"), T.GrandTotal(), 4);
        TestTrue(TEXT("completion 0"), FMath::Abs(T.Completion()) <= Eps);
        TestTrue(TEXT("reward None"), T.Reward() == EReward::None);
    }

    // ---- Collection is idempotent; bad collects are ignored --------------------
    {
        FCollectionTracker T;
        T.DefineCategory(Tags, 4);
        TestTrue(TEXT("first collect is new"), T.Collect(Tags, 0));
        TestFalse(TEXT("re-collecting the same is not new"), T.Collect(Tags, 0));
        TestEqual(TEXT("dup didn't double-count"), T.FoundIn(Tags), 1);
        TestFalse(TEXT("out-of-range index rejected"), T.Collect(Tags, 4));
        TestFalse(TEXT("negative index rejected"), T.Collect(Tags, -1));
        TestFalse(TEXT("unknown category rejected"), T.Collect(TEXT("nope"), 0));
        TestEqual(TEXT("bad collects didn't count"), T.FoundIn(Tags), 1);
    }

    // ---- Per-category fraction + reward tiers across a single category ---------
    {
        FCollectionTracker T;
        T.DefineCategory(Tags, 4);
        T.Collect(Tags, 0);
        TestTrue(TEXT("25% -> Bronze"), T.Reward() == EReward::Bronze);
        TestTrue(TEXT("fraction 0.25"), FMath::IsNearlyEqual(T.FractionIn(Tags), 0.25, Eps));
        T.Collect(Tags, 1);
        TestTrue(TEXT("50% -> Silver"), T.Reward() == EReward::Silver);
        T.Collect(Tags, 2);
        TestTrue(TEXT("75% -> Gold"), T.Reward() == EReward::Gold);
        TestTrue(TEXT("fraction 0.75"), FMath::IsNearlyEqual(T.FractionIn(Tags), 0.75, Eps));
        T.Collect(Tags, 3);
        TestTrue(TEXT("100% -> Platinum"), T.Reward() == EReward::Platinum);
        TestTrue(TEXT("category complete"), T.IsCategoryComplete(Tags));
        TestTrue(TEXT("all complete"), T.IsComplete());
        TestTrue(TEXT("completion 1.0"), FMath::IsNearlyEqual(T.Completion(), 1.0, Eps));
    }

    // ---- Overall completion weights by item count, not category average --------
    {
        FCollectionTracker T;
        T.DefineCategory(Packages, 50);
        T.DefineCategory(Tags, 5);
        for (int32 I = 0; I < 5; ++I)
        {
            T.Collect(Tags, I); // finish the small category entirely
        }
        TestTrue(TEXT("small category is 100%"), FMath::IsNearlyEqual(T.FractionIn(Tags), 1.0, Eps));
        // A naive category-average would read 50%; the real weighted figure is 5/55.
        TestTrue(TEXT("overall is 5/55, count-weighted"),
            FMath::IsNearlyEqual(T.Completion(), 5.0 / 55.0, Eps));
        TestEqual(TEXT("grand total 55"), T.GrandTotal(), 55);
        TestEqual(TEXT("total found 5"), T.TotalFound(), 5);
        TestTrue(TEXT("under 25% -> None"), T.Reward() == EReward::None);
    }

    // ---- A zero-total category is trivially complete and weightless -----------
    {
        FCollectionTracker T;
        T.DefineCategory(Photos, 0);
        TestTrue(TEXT("zero-total fraction is 1"), FMath::IsNearlyEqual(T.FractionIn(Photos), 1.0, Eps));
        TestTrue(TEXT("zero-total is complete"), T.IsCategoryComplete(Photos));
        TestEqual(TEXT("zero-total adds nothing to grand total"), T.GrandTotal(), 0);
        TestFalse(TEXT("can't collect into a zero-total category"), T.Collect(Photos, 0));

        // It doesn't drag down a real category's overall figure.
        T.DefineCategory(Tags, 4);
        T.Collect(Tags, 0);
        TestTrue(TEXT("overall ignores the zero-total category"),
            FMath::IsNearlyEqual(T.Completion(), 0.25, Eps));
    }

    // ---- Re-defining a category resets its progress and total -----------------
    {
        FCollectionTracker T;
        T.DefineCategory(Tags, 4);
        T.Collect(Tags, 0);
        T.Collect(Tags, 1);
        TestEqual(TEXT("found 2 before redefine"), T.FoundIn(Tags), 2);
        T.DefineCategory(Tags, 10); // redefine
        TestEqual(TEXT("still one category"), T.CategoryCount(), 1);
        TestEqual(TEXT("redefine reset found"), T.FoundIn(Tags), 0);
        TestEqual(TEXT("redefine set new total"), T.TotalIn(Tags), 10);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
