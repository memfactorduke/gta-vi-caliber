// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"

struct FPlayerStats;
struct FPlayerHealthModel;

/**
 * GtcDamage — the single damage-routing entry point implementing the W3
 * armor-ownership resolution (docs/W3_WIRING_NOTES.md, "Armor ownership —
 * RESOLVED", commit f0c0c36).
 *
 * The resolution retires the upstream Godot stats->model->health double-soak:
 * the PlayerStats armor pool is the SOLE armor owner; the health model's armor
 * pool is neutralized (the owning UPlayerHealthComponent constructs its
 * FPlayerHealthModel with ArmorMax = 0, so AddArmor clamps to 0 and Apply soaks
 * 0 — it is health-only). A single hit therefore soaks armor EXACTLY ONCE
 * (stats) and the remainder hits health.
 *
 * The LOGIC lives here as a free function so it is headless-testable without a
 * live world: UPlayerStatsComponent / UPlayerHealthComponent / the pawn all
 * route damage through ApplyToPlayer rather than re-implementing the two-step.
 */
namespace GtcDamage
{
    /**
     * Route `Amount` of incoming damage through the resolution:
     *   1. overflow = Stats.SoakDamage(Amount)  (drains the SOLE stats armor pool)
     *   2. Health.Apply(overflow)               (health-only; ArmorMax==0 soaks 0)
     * Returns the overflow that reached the health model (the post-armor amount).
     * Negative `Amount` soaks nothing and applies nothing (Apply ignores it too).
     */
    GTC_UE5_API double ApplyToPlayer(FPlayerStats& Stats, FPlayerHealthModel& Health, double Amount);
}
