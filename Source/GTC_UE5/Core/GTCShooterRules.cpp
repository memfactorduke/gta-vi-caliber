// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCShooterRules.h"

#include "Math/UnrealMathUtility.h"

namespace GTCShooter
{
    FGTCShooterRules Normalize(const FGTCShooterRules& In)
    {
        FGTCShooterRules Out = In;

        // Cash can never be negative.
        Out.StartingCash = FMath::Max(0, In.StartingCash);

        // Health is at least 1 (a 0-HP spawn dies instantly) and capped.
        Out.StartingHealth = FMath::Clamp(In.StartingHealth, 1.0f, MaxStartingHealth);

        // Respawn delay stays within a playable window.
        Out.RespawnDelaySeconds =
            FMath::Clamp(In.RespawnDelaySeconds, MinRespawnDelaySeconds, MaxRespawnDelaySeconds);

        // Spawn protection can be disabled (0) but never negative.
        Out.SpawnProtectionSeconds = FMath::Max(0.0f, In.SpawnProtectionSeconds);

        // bArmPlayerOnSpawn passes through unchanged.
        return Out;
    }
}
