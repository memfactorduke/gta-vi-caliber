// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GTCShooterRules.h"

/**
 * Unit tests for GTCShooter::Normalize — the pure clamp that keeps a designer's
 * shooter game-mode config inside a playable range (no negative cash, no 0-HP
 * spawn, no instant or punitively long respawn).
 */

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGTCShooterRulesNormalizeTest,
    "GTC.Core.ShooterRules.Normalize",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FGTCShooterRulesNormalizeTest::RunTest(const FString& Parameters)
{
    // Below-range / zero / negative inputs clamp up into the playable window.
    {
        FGTCShooterRules In;
        In.StartingCash = -100;
        In.StartingHealth = 0.0f;
        In.RespawnDelaySeconds = 0.0f;
        In.SpawnProtectionSeconds = -5.0f;
        const FGTCShooterRules Out = GTCShooter::Normalize(In);

        TestEqual(TEXT("cash floored at 0"), Out.StartingCash, 0);
        TestTrue(TEXT("health raised to >= 1"), Out.StartingHealth >= 1.0f);
        TestTrue(TEXT("respawn raised to min"),
            Out.RespawnDelaySeconds >= GTCShooter::MinRespawnDelaySeconds);
        TestEqual(TEXT("spawn protection floored at 0"), Out.SpawnProtectionSeconds, 0.0f);
    }

    // Above-range inputs clamp down to the caps.
    {
        FGTCShooterRules In;
        In.StartingHealth = 99999.0f;
        In.RespawnDelaySeconds = 99999.0f;
        const FGTCShooterRules Out = GTCShooter::Normalize(In);

        TestEqual(TEXT("health capped"), Out.StartingHealth, GTCShooter::MaxStartingHealth);
        TestEqual(TEXT("respawn capped"), Out.RespawnDelaySeconds, GTCShooter::MaxRespawnDelaySeconds);
    }

    // Already-sane values pass through unchanged.
    {
        FGTCShooterRules In;
        In.StartingCash = 500;
        In.StartingHealth = 100.0f;
        In.RespawnDelaySeconds = 3.0f;
        In.SpawnProtectionSeconds = 2.0f;
        const FGTCShooterRules Out = GTCShooter::Normalize(In);

        TestEqual(TEXT("cash unchanged"), Out.StartingCash, 500);
        TestEqual(TEXT("health unchanged"), Out.StartingHealth, 100.0f);
        TestEqual(TEXT("respawn unchanged"), Out.RespawnDelaySeconds, 3.0f);
        TestEqual(TEXT("protection unchanged"), Out.SpawnProtectionSeconds, 2.0f);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
