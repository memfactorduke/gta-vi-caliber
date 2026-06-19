// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PlayerHealthTestListener.generated.h"

/**
 * Test-only UObject listener for UPlayerHealthComponent delegates.
 *
 * Dynamic multicast delegates (FGtcHealthChanged etc.) only bind to UFUNCTIONs
 * via AddDynamic — they have no AddLambda — so the lifecycle automation test
 * needs a real UObject with UFUNCTION handlers to count delegate fires. Lives in
 * its own header so UnrealHeaderTool processes the UCLASS (UHT does not scan .cpp).
 *
 * NOTE on the WITH_AUTOMATION_TESTS guard: UHT forbids UCLASS/UFUNCTION inside any
 * preprocessor block other than WITH_EDITORONLY_DATA ("'UCLASS' must not be inside
 * preprocessor blocks") — so a reflected type cannot be wrapped in
 * #if WITH_AUTOMATION_TESTS. This matches the engine's own precedent: every test-only
 * UObject (e.g. Engine/Private/Tests/.../AsyncLoadingTests_Shared.h, TextPropertyTestObject.h)
 * declares its UCLASS unguarded and relies on the Tests/ path being excluded from
 * non-test builds via Build.cs. We cannot edit Build.cs here, so this header is the
 * minimal, inert equivalent: only the WITH_AUTOMATION_TESTS-guarded test .cpp ever
 * includes or instantiates it. Its generated reflection is small and unreferenced
 * outside tests.
 */
UCLASS()
class UPlayerHealthTestListener : public UObject
{
    GENERATED_BODY()

public:
    int32 HealthFires = 0;
    int32 ArmorFires = 0;
    int32 DeathFires = 0;
    float LastHealth = -1.0f;
    float LastMaxHealth = -1.0f;
    float LastArmor = -1.0f;
    float LastMaxArmor = -1.0f;

    UFUNCTION()
    void HandleHealth(float Health, float MaxHealth)
    {
        HealthFires++;
        LastHealth = Health;
        LastMaxHealth = MaxHealth;
    }

    UFUNCTION()
    void HandleArmor(float Armor, float MaxArmor)
    {
        ArmorFires++;
        LastArmor = Armor;
        LastMaxArmor = MaxArmor;
    }

    UFUNCTION()
    void HandleDeath()
    {
        DeathFires++;
    }
};
