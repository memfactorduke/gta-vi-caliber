// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Interactable.generated.h"

/**
 * IGTCInteractable — the contract every context-sensitive interact target
 * implements (shops, hubs, mission actors, pickups). The UE replacement for the
 * Godot "interactables" group: in Godot the player gathered group members and,
 * on the interact key, called interact()/interact_prompt() on the winner. Here
 * any AActor (C++ or Blueprint) that implements this interface is a candidate
 * UGTCInteractionComponent can gather, score (via the merged FInteraction
 * selection model), and act on.
 *
 * All three methods are BlueprintNativeEvent so Blueprint actors and native
 * actors alike supply behaviour; the component calls the Execute_* thunks.
 *
 * GTC-prefixed to stay clear of engine IInteractableTarget/UInteractable types
 * (ODR-checked vs main and the UE 5.7 engine source).
 */
UINTERFACE(MinimalAPI, BlueprintType, meta = (DisplayName = "GTC Interactable"))
class UGTCInteractable : public UInterface
{
    GENERATED_BODY()
};

class GTC_UE5_API IGTCInteractable
{
    GENERATED_BODY()

public:
    /**
     * Perform the interaction. Instigator is the actor that triggered it
     * (typically the player pawn that owns the UGTCInteractionComponent).
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GTC|Interaction")
    void Interact(AActor* Instigator);

    /**
     * Whether this target is currently interactable for the given Instigator.
     * The component gates its selection on this (a candidate that returns false
     * is excluded), so shops/mission actors can refuse out of money, wrong
     * state, etc. Defaults to true in the native body.
     */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GTC|Interaction")
    bool CanInteract(const AActor* Instigator) const;

    /** The context-sensitive prompt text shown for this target ("Press E to ..."). */
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "GTC|Interaction")
    FText GetInteractPrompt() const;
};
