// Copyright (c) 2026 GTC contributors

#if WITH_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"

#include "../World/Interaction/InteractionComponent.h"
#include "../World/Interaction/Interaction.h"
#include "GtcTestTolerances.h"

// What is tested here: the NEW assertable logic of UGTCInteractionComponent —
// the PURE candidate selector UGTCInteractionComponent::SelectBest. Given
// caller-supplied candidate descriptors (world position + a CanInteract flag),
// it must pick exactly the target the merged, W1-tested FInteraction model
// (World/Interaction/Interaction.h) selects over the CAN-INTERACT subset, and
// the CanInteract flag must GATE candidates (a false candidate is excluded and
// can never win, even when it is the geometric nearest).
//
// The selection is integer-index identity, so the picks need no float tolerance;
// GtcTest::Eps is used only to corroborate the geometry the pick rests on (the
// shared, double, ODR-safe tolerance — no local Eps).
//
// DEFERRED-TO-PIE: the component's LIVE gather (GatherCandidates / the tick
// overlap+trace in UpdateSelection, and the IGTCInteractable::Execute_* calls on
// real actors) needs a UWorld and spawned actors, so it is NOT unit-tested here
// — it is verified in PIE, exactly the Wave-3 adapter boundary. These tests
// drive the scene-free selector directly and do NOT fake-assert the live trace.

namespace
{
    // System-prefixed helper (no generic MakeSample): build one candidate.
    FGtcInteractionCandidate GtcMakeInteractionCandidate(const FVector& Location, bool bCanInteract)
    {
        FGtcInteractionCandidate Candidate;
        Candidate.Location = Location;
        Candidate.bCanInteract = bCanInteract;
        Candidate.Actor = nullptr; // headless: the pick is an index, no actor needed
        return Candidate;
    }
}

BEGIN_DEFINE_SPEC(FGtcInteractionComponentSpec, "GTC.World.InteractionComponent",
    EAutomationTestFlags::ProductFilter | EAutomationTestFlags_ApplicationContextMask)
END_DEFINE_SPEC(FGtcInteractionComponentSpec)

void FGtcInteractionComponentSpec::Define()
{
    // No candidates -> nothing selected.
    It("selects nothing from an empty candidate set", [this]()
    {
        const TArray<FGtcInteractionCandidate> Candidates;
        TestEqual(TEXT("empty candidate set selects None"),
            UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0),
            UGTCInteractionComponent::None);
    });

    // A single in-reach, interactable candidate is selected.
    It("selects a lone in-reach interactable candidate", [this]()
    {
        const TArray<FGtcInteractionCandidate> Candidates{
            GtcMakeInteractionCandidate(FVector(2, 0, 0), /*bCanInteract=*/true) };
        TestEqual(TEXT("the only valid candidate is picked"),
            UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0), 0);
    });

    // The pick matches the merged FInteraction model over the same geometry: of
    // several can-interact candidates the nearest within reach wins, and the
    // index is the SAME one FInteraction::Nearest returns.
    It("picks the FInteraction-nearest of several interactable candidates", [this]()
    {
        const TArray<FVector> Geometry{ FVector(2.5, 0, 0), FVector(1.0, 0, 0), FVector(2.0, 0, 0) };
        TArray<FGtcInteractionCandidate> Candidates;
        for (const FVector& P : Geometry)
        {
            Candidates.Add(GtcMakeInteractionCandidate(P, /*bCanInteract=*/true));
        }
        const int32 Picked = UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0);
        // Component pick == merged FInteraction pick over identical geometry.
        TestEqual(TEXT("component picks the FInteraction winner index"),
            Picked, FInteraction::Nearest(Geometry, FVector::ZeroVector, 3.0));
        TestEqual(TEXT("that winner is the nearest candidate (index 1)"), Picked, 1);
        // Corroborate the geometry the pick rests on with the shared tolerance.
        TestTrue(TEXT("winner distance is the minimum"),
            FMath::Abs(FVector::Dist(FVector::ZeroVector, Geometry[Picked]) - 1.0) < GtcTest::Eps);
    });

    // CanInteract GATES: the geometric nearest is excluded when it cannot
    // interact, so the next can-interact candidate wins instead.
    It("excludes the nearest when it cannot interact (CanInteract gating)", [this]()
    {
        TArray<FGtcInteractionCandidate> Candidates;
        Candidates.Add(GtcMakeInteractionCandidate(FVector(1.0, 0, 0), /*bCanInteract=*/false)); // nearest, gated out
        Candidates.Add(GtcMakeInteractionCandidate(FVector(2.0, 0, 0), /*bCanInteract=*/true));  // next nearest, valid
        const int32 Picked = UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0);
        TestEqual(TEXT("gated-out nearest is skipped; the valid candidate wins"), Picked, 1);
    });

    // When EVERY candidate is gated out, nothing is selected even though one is
    // geometrically in reach.
    It("selects nothing when all candidates are gated out", [this]()
    {
        TArray<FGtcInteractionCandidate> Candidates;
        Candidates.Add(GtcMakeInteractionCandidate(FVector(1.0, 0, 0), /*bCanInteract=*/false));
        Candidates.Add(GtcMakeInteractionCandidate(FVector(2.0, 0, 0), /*bCanInteract=*/false));
        TestEqual(TEXT("all-gated set selects None"),
            UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0),
            UGTCInteractionComponent::None);
    });

    // Gating preserves the original index mapping: when an EARLIER candidate is
    // gated out, the returned index must still refer to the caller's array, not
    // the compacted subset.
    It("returns the original (caller-array) index after gating compaction", [this]()
    {
        TArray<FGtcInteractionCandidate> Candidates;
        Candidates.Add(GtcMakeInteractionCandidate(FVector(0.5, 0, 0), /*bCanInteract=*/false)); // 0: nearest, gated
        Candidates.Add(GtcMakeInteractionCandidate(FVector(5.0, 0, 0), /*bCanInteract=*/true));  // 1: out of reach
        Candidates.Add(GtcMakeInteractionCandidate(FVector(2.0, 0, 0), /*bCanInteract=*/true));  // 2: valid winner
        const int32 Picked = UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0);
        TestEqual(TEXT("winner maps back to original index 2"), Picked, 2);
    });

    // Out-of-reach candidates select nothing (reach delegated to FInteraction).
    It("selects nothing when the only candidate is out of reach", [this]()
    {
        const TArray<FGtcInteractionCandidate> Candidates{
            GtcMakeInteractionCandidate(FVector(10, 0, 0), /*bCanInteract=*/true) };
        TestEqual(TEXT("out-of-reach candidate selects None"),
            UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0),
            UGTCInteractionComponent::None);
    });

    // Ties resolve to the lower ORIGINAL index, matching FInteraction's stable tie-break.
    It("resolves equidistant candidates to the lower original index", [this]()
    {
        TArray<FGtcInteractionCandidate> Candidates;
        Candidates.Add(GtcMakeInteractionCandidate(FVector(2, 0, 0), /*bCanInteract=*/true));
        Candidates.Add(GtcMakeInteractionCandidate(FVector(0, 0, 2), /*bCanInteract=*/true));
        TestEqual(TEXT("equal distances keep the first candidate"),
            UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 3.0), 0);
    });

    // Non-positive reach selects nothing (delegated to FInteraction's reach rule).
    It("selects nothing for a non-positive reach", [this]()
    {
        const TArray<FGtcInteractionCandidate> Candidates{
            GtcMakeInteractionCandidate(FVector::ZeroVector, /*bCanInteract=*/true) };
        TestEqual(TEXT("zero reach selects None"),
            UGTCInteractionComponent::SelectBest(Candidates, FVector::ZeroVector, 0.0),
            UGTCInteractionComponent::None);
    });
}

#endif // WITH_AUTOMATION_TESTS
