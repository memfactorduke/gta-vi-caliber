// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "ArrestTestListener.generated.h"

/**
 * Test-only UObject listener for UArrestSubsystem::OnBusted.
 *
 * A dynamic multicast delegate (FOnArrestBusted) only binds to UFUNCTIONs via AddDynamic —
 * it has no AddLambda — so the bust-broadcast behavior test needs a real UObject with a
 * UFUNCTION handler to capture the broadcast fee. Lives in its own header so UnrealHeaderTool
 * processes the UCLASS (UHT does not scan .cpp files).
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
 * (Mirrors the existing project precedent Player/Stats/Tests/PlayerStatsTestListener.h.)
 */
UCLASS()
class UArrestTestListener : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    bool bFired = false;

    UPROPERTY()
    int32 ReceivedFee = -1;

    UFUNCTION()
    void HandleBusted(int32 Fee)
    {
        bFired = true;
        ReceivedFee = Fee;
    }
};
