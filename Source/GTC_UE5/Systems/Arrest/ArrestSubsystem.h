// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ArrestModel.h"
#include "ArrestSubsystem.generated.h"

/**
 * Live bridge for FArrestModel — the UE 5.7 port of Godot's self-wiring ArrestController
 * Node. Drives the pure, tested busted state machine: while the player is wanted and an
 * officer corners them (within catch range), a grapple timer builds; once it holds for
 * GrappleTime the bust lands — the fee is taken, the heat is cleared, and the player is
 * hauled to a station — and the OnBusted delegate fires with the fee.
 *
 * Lifecycle: a UGameInstanceSubsystem (the bust loop is player-global, like the Godot
 * group "arrest"). It owns a single grapple-timer / busted state for its whole lifetime.
 *
 * OWNERSHIP MODEL: ArrestSubsystem OWNS the pure FArrestModel math (static helpers) and
 * the per-frame grapple state (_TimeHeld). The Godot scene-graph wiring — resolving the
 * player/police Node3D, nearest-cop planar distance, respawn to a spawn_points node — is
 * Wave 3 engine wiring. Here the pure tick math is preserved as explicit driver methods
 * (Tick) that a future scene adapter feeds (stars, nearest-cop distance) into, so behavior
 * stays headless-testable.
 *
 * DEFERRED-OWNERSHIP (lead-signed, option 1 — own-state, defer write-through):
 *   The arrest-from-wanted coupling (a bust triggered by wanted level / police proximity)
 *   is INTENTIONALLY decoupled here. This subsystem does NOT include or depend on any
 *   Wanted / police / PlayerStats type (those live in sibling worktrees / Wave 3). Instead:
 *     - The wanted "stars" and nearest-cop distance are PASSED IN to Tick() by the caller;
 *       this subsystem never reaches for a WantedTracker.
 *     - On a bust, the heat-clear and the cash spend are NOT written through to a Wanted /
 *       PlayerStats system. Instead the subsystem OWNS the snapshot it needs: it holds the
 *       player's wallet (SetWallet/GetWallet) and computes/exposes the fee (GetLastFee) and
 *       the "should clear heat" intent (bClearHeatRequested / ConsumeClearHeatRequest), to
 *       be drained by the Wave 3 adapter that owns the real Wanted/PlayerStats write.
 *   The trigger wiring (binding real police positions + real wanted stars into Tick, and
 *   applying the fee/heat-clear to the real systems) is DEFERRED to integration. This is a
 *   knowing deferred coupling, recorded here for lead sign-off.
 */

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnArrestBusted, int32, Fee);

UCLASS()
class GTC_UE5_API UArrestSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    /** How close an officer must be to start cuffing the player (Godot @export catch_distance). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrest")
    double CatchDistance = 2.0;

    /** Seconds cornered before the bust lands (Godot @export grapple_time). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrest")
    double GrappleTime = FArrestModel::DefaultGrappleTime;

    /** Fraction of the wallet forfeited on a bust (Godot @export cash_penalty). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Arrest")
    double CashPenalty = FArrestModel::DefaultCashPenalty;

    /** Fires when a bust lands, with the cash fee taken (Godot `busted(fee)` signal). */
    UPROPERTY(BlueprintAssignable, Category = "Arrest")
    FOnArrestBusted OnBusted;

    // USubsystem lifecycle.
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

    /**
     * Advance one physics frame. `Stars` is the player's wanted level and `NearestCopDistance`
     * the planar distance to the closest living officer — both PASSED IN (deferred coupling:
     * this subsystem never reaches for a Wanted/police system). Builds/decays the grapple timer
     * via FArrestModel and applies the bust when it lands. Mirrors ArrestController._physics_process.
     * Returns true on the frame the bust lands.
     */
    UFUNCTION(BlueprintCallable, Category = "Arrest")
    bool Tick(int32 Stars, double NearestCopDistance, double DeltaSeconds);

    /** 0 -> 1 as the cuffs close in, for a HUD prompt. Mirrors ArrestController.grapple_progress. */
    UFUNCTION(BlueprintCallable, Category = "Arrest")
    double GrappleProgress() const;

    /** Current grapple timer in seconds (owned bust state). */
    double GetTimeHeld() const { return _TimeHeld; }

    /** Reset the grapple timer to zero (e.g. a forced state reset). */
    void ResetGrapple() { _TimeHeld = 0.0; }

    // --- Owned wallet snapshot (deferred PlayerStats coupling) ------------------

    /** Set the owned wallet snapshot the fee is computed against. */
    UFUNCTION(BlueprintCallable, Category = "Arrest")
    void SetWallet(int32 InWallet) { _Wallet = FMath::Max(InWallet, 0); }

    /** The owned wallet snapshot (after any bust fee already deducted). */
    int32 GetWallet() const { return _Wallet; }

    /** The fee taken on the most recent bust (0 if none yet). */
    UFUNCTION(BlueprintCallable, Category = "Arrest")
    int32 GetLastFee() const { return _LastFee; }

    // --- Owned heat-clear intent (deferred Wanted coupling) --------------------

    /** True if a bust has requested a heat clear that the Wave 3 adapter has not yet drained. */
    bool IsClearHeatRequested() const { return bClearHeatRequested; }

    /** Drain the pending heat-clear request (returns its prior value, then clears it). */
    bool ConsumeClearHeatRequest()
    {
        const bool bWas = bClearHeatRequested;
        bClearHeatRequested = false;
        return bWas;
    }

private:
    /** Apply the bust: take the fee from the owned wallet, request a heat clear, fire OnBusted. */
    void ApplyBust();

    /** Grapple timer, seconds (Godot _time_held). */
    double _TimeHeld = 0.0;

    /** Owned wallet snapshot (deferred PlayerStats coupling). */
    int32 _Wallet = 0;

    /** Fee taken on the most recent bust. */
    int32 _LastFee = 0;

    /** Pending "clear heat" intent from a bust, drained by the Wave 3 Wanted adapter. */
    bool bClearHeatRequested = false;
};
