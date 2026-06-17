// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure target-selection for the player's context-sensitive interact key: given the
 * world positions of nearby interactables and where the player is, pick the index
 * of the one to act on. Scene-free, so it unit-tests headless against the parity
 * oracle (World/Interaction/Tests/InteractionTest.cpp).
 *
 * Ported 1:1 from the Godot RefCounted `Interaction`
 * (game/scripts/world/interaction.gd). Plain value type, no UObject — pure static
 * helper.
 *
 * PURE-CORE-ONLY-IS-TESTED / Wave-3 boundary: the tested unit is JUST the
 * pick-the-nearest-in-reach scoring below. In Godot the player gathers the
 * candidate interactables (Node3D members of group "interactables"), and on a key
 * press calls interact()/interact_prompt() on the winner. Gathering the
 * candidates (a UE line trace / overlap query / AI Perception sweep), reading
 * their world positions, and the Enhanced Input wiring that fires interact() are
 * the deferred Wave-3 adapter — NOT implemented or tested here. This core takes
 * the candidate positions as a plain caller-supplied array of points.
 *
 * Axis convention (Godot, Y-up): positions live on the XZ ground plane (Y up).
 * No UE Z-up remap is baked in — `Nearest` uses a full 3D Euclidean distance
 * (FVector::Dist) exactly as the Godot `Vector3.distance_to`, so the ported unit
 * tests match the oracle bit-for-bit. Remapping to UE's Z-up space is the same
 * deferred Wave-3 concern that the adapter owns.
 *
 * Double precision throughout (LWC): FVector is double-backed, matching the
 * Godot float math.
 */
class GTC_UE5_API FInteraction
{
public:
    /** Returned by Nearest when nothing qualifies. */
    static constexpr int32 None = -1;

    /**
     * Index of the closest point within `Reach` metres of `From`, or None.
     * The reach boundary is inclusive; ties resolve to the lower index (stable);
     * a non-positive reach selects nothing.
     */
    static int32 Nearest(const TArray<FVector>& Points, const FVector& From, double Reach);
};
