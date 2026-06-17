// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * FDisguise — pure appearance / disguise model (UE 5.7 port of the reference `disguise.gd`,
 * class Disguise, RefCounted). Parity oracle game/tests/unit/test_disguise.gd (11 funcs).
 *
 * The genre's "change your look to lose the heat" mechanic. The player has an appearance
 * across weighted slots (outfit, mask, vehicle, hair); when police get a good look they
 * LOG a description, and how much your current look still matches that description is your
 * recognition. Swap enough slots and recognition drops, so a searching patrol gives up
 * faster. The slot weights sum to 1.0 so recognition lands in [0, 1]. double precision for
 * float parity with the oracle.
 *
 * Ordered backing store: the reference WEIGHTS is a Dictionary whose key order (outfit, mask,
 * vehicle, hair) is observable through slots(); we preserve that order with a parallel
 * ordered list of slot names rather than relying on TMap iteration order.
 *
 * DEFERRED-OWNERSHIP (option-1 own-state) — lead sign-off required:
 *   The disguise-from-wanted coupling (a disguised player draining WantedEvasion's "go
 *   cold" countdown faster) is a DEFERRED-OWNERSHIP decision. This model OWNS its own
 *   disguise state and exposes effectiveness only via EvasionSpeedup(); a caller multiplies
 *   the WantedEvasion search delta by it. FDisguise does NOT reach into any live
 *   WantedSubsystem and does not import it for production use. The reference behavior's
 *   cross-system test (`test_disguised_evades_faster_than_recognized`) drives a standalone
 *   FWantedEvasion (reused from main, NOT re-ported) purely to assert the speedup math; the
 *   runtime read-back / wiring into the live wanted system is a Wave-3 adapter and DEFERRED.
 *   The the reference disguise_tracker.gd (Node) and the runtime wanted read-back are likewise
 *   DEFERRED to Wave 3 and intentionally not ported here.
 */
class GTC_UE5_API FDisguise
{
public:
    /** How much faster a fully-disguised player (recognition 0) drains the search countdown
     *  versus a fully-recognized one (recognition 1 -> 1.0x). */
    static constexpr double MaxEvasionSpeedup = 3.0;

    /** Value every slot starts at (and resets to). the reference DEFAULT_LOOK. */
    static const FString DefaultLook;

    /** Construct with every appearance slot set to DefaultLook (Godot _init). */
    FDisguise();

    /** Every appearance slot, in weight-declaration order (outfit, mask, vehicle, hair). */
    TArray<FString> Slots() const;

    /** The player's current value for a slot, or "" for an unknown slot. */
    FString Current(const FString& Slot) const;

    /** Change one appearance slot. Unknown slots are ignored. */
    void SetAppearance(const FString& Slot, const FString& Value);

    /** Police memorise the player's current look as the description to hunt. */
    void LogSighting();

    /** True once police have a description on file. */
    bool HasDescription() const;

    /**
     * How recognizable the player is versus the logged description, in [0, 1]: the summed
     * weight of every slot that still matches. 0.0 when police have no description.
     */
    double Recognition() const;

    /**
     * Multiplier a caller applies to the WantedEvasion search delta: 1.0 when fully
     * recognized up to MaxEvasionSpeedup when fully disguised.
     */
    double EvasionSpeedup() const;

    /** True if the player still matches the description at or above `Threshold`. */
    bool IsRecognized(double Threshold) const;

    /** How many slots differ from the logged description (0 when none is on file). */
    int32 ChangedSlots() const;

    /** Clear the logged description (police lose the trail once you go cold). */
    void ResetToClean();

private:
    /** Weight of one slot in recognition. Returns 0 for unknown slots. */
    static double WeightOf(const FString& Slot);

    /** Slot names in the reference WEIGHTS declaration order. */
    static const TArray<FString>& OrderedSlots();

    /** slot -> current value the player is wearing/driving. */
    TMap<FString, FString> _Current;

    /** slot -> value police last logged. Empty until the first LogSighting(). */
    TMap<FString, FString> _WantedLook;

    /** Whether a description has been logged (mirrors the reference empty-dict check). */
    bool _HasWantedLook = false;
};
