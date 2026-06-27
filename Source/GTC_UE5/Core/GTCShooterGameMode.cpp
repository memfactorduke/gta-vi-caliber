// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCShooterGameMode.h"

AGTCShooterGameMode::AGTCShooterGameMode()
{
    // Pawn / controller / HUD are assigned by the Blueprint child so the paid-kit
    // references stay out of this public C++ (see class comment).
}

void AGTCShooterGameMode::BeginPlay()
{
    Super::BeginPlay();

    FGTCShooterRules Raw;
    Raw.StartingCash = StartingCash;
    Raw.StartingHealth = StartingHealth;
    Raw.RespawnDelaySeconds = RespawnDelaySeconds;
    Raw.SpawnProtectionSeconds = SpawnProtectionSeconds;
    Raw.bArmPlayerOnSpawn = bArmPlayerOnSpawn;
    Rules = GTCShooter::Normalize(Raw);

    UE_LOG(LogTemp, Log,
        TEXT("[GTCShooter] Rules: cash=%d health=%.0f respawn=%.1fs protect=%.1fs arm=%d weapons=%d"),
        Rules.StartingCash, Rules.StartingHealth, Rules.RespawnDelaySeconds,
        Rules.SpawnProtectionSeconds, Rules.bArmPlayerOnSpawn ? 1 : 0, StartingWeapons.Num());
}
