// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "PlayerStatsTestListener.generated.h"

/**
 * Test-only UObject listener for UPlayerStatsComponent delegates.
 *
 * Dynamic multicast delegates (FGtcArmorChanged etc.) only bind to UFUNCTIONs via
 * AddDynamic — they have no AddLambda — so the lifecycle automation test needs a
 * real UObject with UFUNCTION handlers to count delegate fires. Lives in its own
 * header so UnrealHeaderTool processes the UCLASS (UHT does not scan .cpp files).
 *
 * NOTE on the WITH_AUTOMATION_TESTS guard: UHT forbids UCLASS/UFUNCTION inside any
 * preprocessor block other than WITH_EDITORONLY_DATA ("'UCLASS' must not be inside
 * preprocessor blocks") — so a reflected type cannot be wrapped in
 * #if WITH_AUTOMATION_TESTS. This matches the engine's own precedent: every test-only
 * UObject (e.g. Engine/Private/Tests/.../AsyncLoadingTests_Shared.h, TextPropertyTestObject.h,
 * PieFixupTestObjects.h) declares its UCLASS unguarded and relies on the Tests/ path being
 * excluded from non-test builds via Build.cs. We cannot edit Build.cs here, so this header is
 * the minimal, inert equivalent: only the WITH_AUTOMATION_TESTS-guarded test .cpp ever includes
 * or instantiates it. Its generated reflection is small and unreferenced outside tests.
 */
UCLASS()
class UPlayerStatsTestListener : public UObject
{
    GENERATED_BODY()

public:
    int32 ArmorFires = 0;
    int32 MoneyFires = 0;
    int32 ObjectiveFires = 0;
    float LastArmor = -1.0f;
    int32 LastMoney = -1;

    UFUNCTION()
    void HandleArmor(float Armor, float MaxArmor)
    {
        ArmorFires++;
        LastArmor = Armor;
    }

    UFUNCTION()
    void HandleMoney(int32 Amount)
    {
        MoneyFires++;
        LastMoney = Amount;
    }

    UFUNCTION()
    void HandleObjective(const FString& Title, bool bHasWaypoint)
    {
        ObjectiveFires++;
    }
};
