// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "InteractionComponent.generated.h"

class AActor;

/**
 * UGTCInteractionComponent — the player-side interaction adapter. The UE
 * replacement for the Godot player's interact handling: each tick it gathers the
 * nearby IGTCInteractable actors in front of the owner (forward sphere/overlap +
 * trace), scores them with the merged FInteraction selection model
 * (World/Interaction/Interaction.h — the W1-tested "nearest in reach" pick), and
 * exposes the selected target + its prompt. On the IA_Interact action the owning
 * player character calls TriggerInteract(), which fires Interact() on the
 * selected target.
 *
 * ---------------------------------------------------------------------------
 * TWO LAYERS (mirroring the project's model/component split):
 *
 *  - SelectBest(...) : a PURE static selector over caller-supplied candidate
 *    descriptors (world position + a CanInteract flag). It delegates the
 *    geometry to FInteraction::Nearest on the can-interact subset, so the choice
 *    is the SAME math the W1 parity tests pin. Scene-free => headless-testable
 *    (Tests/InteractionComponentTest.cpp). This is the NEW assertable logic.
 *
 *  - the live gather (GatherCandidates / UpdateSelection on tick) : the thin
 *    UE adapter that runs the overlap/trace against a real UWorld and builds the
 *    descriptors. It needs a world, so it is DEFERRED-TO-PIE (not unit-tested) —
 *    exactly the Wave-3 adapter boundary FInteraction.h documents.
 *
 * ---------------------------------------------------------------------------
 * GTC-prefixed (UGTCInteractionComponent / FGtcInteractionCandidate /
 * delegates) to avoid the engine's UInteractionComponent etc. ODR-checked vs
 * main and the UE 5.7 engine source. Selection math is double-precision via
 * FInteraction; no local Eps.
 */

/** Selected interactable changed -> (NewTarget). Null when nothing is selected. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FGtcInteractTargetChanged, AActor*, NewTarget);

/**
 * One scored candidate, as the pure selector sees it. The live adapter builds
 * these from overlap/trace hits; the tests build them directly. Pure value type,
 * no UObject dependency beyond the (opaque, possibly null in tests) Actor handle.
 */
USTRUCT()
struct FGtcInteractionCandidate
{
    GENERATED_BODY()

    /** World position used for the FInteraction distance scoring. */
    UPROPERTY()
    FVector Location = FVector::ZeroVector;

    /** IGTCInteractable::CanInteract gate result; false candidates are excluded. */
    UPROPERTY()
    bool bCanInteract = true;

    /** The candidate actor (may be null in headless tests that only pin the pick). */
    UPROPERTY()
    TObjectPtr<AActor> Actor = nullptr;
};

UCLASS(ClassGroup = (GTC), meta = (BlueprintSpawnableComponent))
class GTC_UE5_API UGTCInteractionComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UGTCInteractionComponent();

    /** Returned by SelectBest when nothing qualifies (no candidate / all gated). */
    static constexpr int32 None = -1;

    /**
     * PURE SELECTOR (the tested unit). Of the candidates whose bCanInteract is
     * true, returns the index of the one the merged FInteraction model picks for
     * an interactor at `From` with the given `Reach` — i.e. nearest within reach,
     * ties to the lower ORIGINAL index, non-positive reach selects nothing.
     * Returns None when no can-interact candidate is in reach.
     *
     * Defers the geometry to FInteraction::Nearest (W1-tested) over the gated
     * subset, then maps the subset winner back to the original index so the
     * CanInteract gating cannot change which geometry the FInteraction math runs.
     */
    static int32 SelectBest(const TArray<FGtcInteractionCandidate>& Candidates,
        const FVector& From, double Reach);

    // --- Live selection state (populated by the deferred-to-PIE adapter) --------

    /** The currently selected interactable actor, or null. */
    UFUNCTION(BlueprintPure, Category = "GTC|Interaction")
    AActor* GetSelectedTarget() const { return SelectedTarget; }

    /** Whether a target is currently selected. */
    UFUNCTION(BlueprintPure, Category = "GTC|Interaction")
    bool HasSelection() const { return SelectedTarget != nullptr; }

    /** The selected target's prompt (empty when nothing is selected). */
    UFUNCTION(BlueprintPure, Category = "GTC|Interaction")
    FText GetSelectedPrompt() const;

    /** Fired whenever the selected target changes (incl. to/from null). */
    UPROPERTY(BlueprintAssignable, Category = "GTC|Interaction")
    FGtcInteractTargetChanged OnSelectedTargetChanged;

    /** Reach radius (cm) of the forward gather/score. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GTC|Interaction")
    float InteractReach = 250.0f;

    /**
     * Fire Interact() on the currently selected target (re-checking CanInteract).
     * Bound to IA_Interact by the owning player character. Returns true if a
     * target was interacted with. Safe to call with no selection (returns false).
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Interaction")
    bool TriggerInteract();

    /**
     * Re-run the live gather+score and update SelectedTarget. DEFERRED-TO-PIE:
     * the gather uses the owner's UWorld (overlap/trace), so this is exercised in
     * PIE, not in the headless tests. The pure pick it ends in is SelectBest,
     * which IS tested.
     */
    UFUNCTION(BlueprintCallable, Category = "GTC|Interaction")
    void UpdateSelection();

    virtual void TickComponent(float DeltaTime, ELevelTick TickType,
        FActorComponentTickFunction* ThisTickFunction) override;

private:
    /**
     * Live overlap/trace against the owner's world to build candidate
     * descriptors. DEFERRED-TO-PIE (needs a UWorld); not unit-tested.
     */
    void GatherCandidates(TArray<FGtcInteractionCandidate>& OutCandidates, FVector& OutFrom) const;

    /** Set the selection and broadcast if it changed. */
    void SetSelectedTarget(AActor* NewTarget);

    UPROPERTY(Transient)
    TObjectPtr<AActor> SelectedTarget = nullptr;
};
