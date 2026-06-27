// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

/**
 * Pure, non-UObject rules for the GTA-style shooter game mode. Kept free of any
 * engine-asset dependency so it is unit-testable headless (pattern:
 * Player/PlayerMotion). AGTCShooterGameMode reads its designer-facing config into
 * an FGTCShooterRules and Normalize()s it once at BeginPlay, so a typo (negative
 * cash, zero health, instant respawn) can never push the match into an invalid,
 * unplayable state.
 */
struct FGTCShooterRules
{
    /** Cash a brand-new player starts a session with. */
    int32 StartingCash = 500;

    /** Health the player (re)spawns with. */
    float StartingHealth = 100.0f;

    /** Seconds between death and respawn. */
    float RespawnDelaySeconds = 3.0f;

    /** Seconds of damage immunity granted right after a (re)spawn. */
    float SpawnProtectionSeconds = 2.0f;

    /** Whether the player is handed the starting arsenal on spawn. */
    bool bArmPlayerOnSpawn = true;
};

namespace GTCShooter
{
    /** Upper clamp on starting health so a stray config can't make a bullet sponge. */
    inline constexpr float MaxStartingHealth = 1000.0f;

    /** Respawn must not be instant (camera/anim need a beat) nor punitively long. */
    inline constexpr float MinRespawnDelaySeconds = 0.5f;
    inline constexpr float MaxRespawnDelaySeconds = 30.0f;

    /** Return a copy of In with every field clamped into a sane, playable range. */
    FGTCShooterRules Normalize(const FGTCShooterRules& In);
}
