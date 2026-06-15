// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure PAY-N-SPRAY / hideout instant wanted-loss model — the iconic open-world respray.
 *
 * Duck into a spray shop or safehouse and, IF no cop saw you enter, the wanted
 * level clears the moment the door shuts. This is the INSTANT-CLEAR path and is
 * deliberately distinct from WantedEvasion's slow "go cold" countdown.
 *
 * Plain C++ value type (no UObject): static eligibility checks (CanEnter /
 * IsSeenEntering / CostFor) plus a tiny stateful respray timer (the seconds spent
 * inside with the door shut), so the whole mechanic is unit-tested headless.
 * A node feeds it the player/shop/police positions, the wallet balance, and delta.
 *
 * Parity note: line traces / overlap / AI-Perception LOS belong to a Wave 3 thin
 * adapter; IsSeenEntering here is the pure radius test only.
 */
class GTC_UE5_API PaySpray
{
public:
    /**
     * Heat cap. Mirrors WantedSystem::MaxStars; kept local to stay decoupled from
     * the Police builder (CostFor clamps stars into [0, MaxStars]).
     */
    static constexpr int32 MaxStars = 5;

    /** Default per-star respray surcharge when a caller doesn't supply one. */
    static constexpr int32 DefaultPerStar = 100;

    /**
     * True when the player is at the shop entrance (within Radius). A negative
     * radius is treated as zero, so a bad value can never wave the player in.
     */
    static bool CanEnter(const FVector& Player, const FVector& Shop, float Radius);

    /**
     * Respray price for the current heat: higher stars cost more. 0 stars -> 0
     * (nothing to lose, nothing to pay). Stars are clamped to [0, MaxStars] and
     * Base / PerStar floored at 0 so the price is never negative.
     */
    static int32 CostFor(int32 Stars, int32 Base, int32 PerStar = DefaultPerStar);

    /**
     * True if any cop is close enough to the shop entrance to watch you duck in
     * (within Sight). If so the respray WON'T clear — you've been traced.
     */
    static bool IsSeenEntering(const FVector& Shop, const TArray<FVector>& Police, float Sight);

    /** Outcome of a respray attempt against the wallet. */
    struct FResolveResult
    {
        bool bAllowed = false;
        int32 Cost = 0;
        int32 NewBalance = 0;
        FString Reason;
    };

    /**
     * Construct an idle respray timer. Duration is floored at 0.001 so Progress()
     * never divides by zero and the respray always completes in finite time.
     */
    explicit PaySpray(float Duration = 3.0f);

    /** Begin the respray timer from scratch (the door just shut behind you). */
    void Begin();

    /**
     * Advance the respray. No-op before Begin(), after completion, or for a
     * non-positive delta, so a stray frame can't push a stale timer forward.
     */
    void Tick(float Delta);

    /** 0.0 at begin, ramping to 1.0 when the respray finishes. 0.0 before Begin(). */
    float Progress() const;

    /** True once the car has been resprayed long enough — wanted level may clear. */
    bool IsComplete() const;

    /** Abort an in-progress respray (you bailed before the job finished). */
    void Cancel();

    /** Back to the idle / pre-begin state. */
    void Reset();

    /**
     * Resolve a respray attempt against the wallet. Pure: the caller applies
     * NewBalance and, on allowed, clears the wanted level. Fails when: 0 stars
     * (nothing to lose), a cop saw you enter (traced), or the balance can't cover
     * the cost.
     */
    FResolveResult Resolve(int32 Stars, int32 Balance, bool bSeen, int32 Base, int32 PerStar = DefaultPerStar) const;

private:
    float ResprayDuration = 3.0f;
    bool bRunning = false;
    float Time = 0.0f;

    static FResolveResult Deny(int32 Balance, const FString& Reason);
};
