// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

/**
 * Pure pursuit-memory for a chasing officer: where to move when the target is in
 * or out of sight, and when to abandon the chase. This is what makes the "go
 * cold" evasion loop real — without it police steer to the player's exact live
 * position every tick.
 *
 * Direct port of the Godot `PursuitMemory` (RefCounted) at
 * `game/scripts/ai/pursuit_memory.gd`. All static, FVector-in,
 * FVector/enum/bool-out, no UObject — unit-tested headless via the parity oracle
 * (Tests/PursuitMemoryTest.cpp, prefix GTC.AI.Pursuit.PursuitMemory).
 *
 * Double precision throughout, to match the GDScript float math. "Planar" is the
 * XZ plane (Godot Y-up). No Godot->UE Z-up axis remap — the model stays in the
 * Godot XZ frame so tests match the oracle bit-for-bit; the axis convention is a
 * DEFERRED Wave-3 concern.
 *
 * PURE-CORE boundary: the pure decision/memory math only — caller supplies
 * positions, seen-flag, timers and the area-uniform RNG samples (u_radius,
 * u_angle). AI Perception / line-of-sight sensing and Behavior Tree wiring are
 * the deferred Wave-3 adapter, NOT implemented here and NOT covered by tests.
 */

/** Chase classification. Mirrors the Godot `State` enum 1:1 (same order). */
enum class EPursuitMemoryState : uint8
{
    Pursue,
    Search,
    Lost,
};

struct GTC_UE5_API FPursuitMemory
{
    /** Default radius an officer mills around the last-known point while searching. */
    static constexpr double SearchRadius = 6.0;

    /** Where to steer: live target while in sight, else its last-known spot. */
    static FVector Target(bool bSeen, const FVector& TargetPos, const FVector& LastKnown);

    /** True once out of sight long enough to abandon. <=0 disables (relentless). */
    static bool ShouldGiveUp(double TimeUnseen, double GiveUpTime);

    /** Classify the chase from sight, unseen-timer, and reached-last-known flags. */
    static EPursuitMemoryState State(
        bool bSeen, double TimeUnseen, bool bReachedLastKnown, double GiveUpTime);

    /** Area-uniform sweep point near the last-known position. u in [0, 1). */
    static FVector SearchPoint(
        const FVector& LastKnown, double URadius, double UAngle, double Radius = SearchRadius);
};
