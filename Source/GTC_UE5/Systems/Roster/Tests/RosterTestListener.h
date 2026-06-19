// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "RosterTestListener.generated.h"

/**
 * Test-only listener for UCharacterRosterSubsystem::OnCharacterSwitched, which is a DYNAMIC
 * multicast delegate (BlueprintAssignable) and therefore can only bind a UFUNCTION via
 * AddDynamic — a plain lambda can't subscribe. A UObject with a UFUNCTION handler captures
 * the broadcast so the behavior test can assert it fired with the right id.
 *
 * UHT forbids `#if`-guarding a UCLASS, so this listener is UNGUARDED here in Tests/. It is
 * a tiny, test-support-only UObject; it is never referenced by shipping code. System-prefixed
 * name (URosterTestSwitchListener) keeps it unique vs main and sibling test helpers (ODR).
 */
UCLASS()
class URosterTestSwitchListener : public UObject
{
    GENERATED_BODY()

public:
    int32 HeardCount = 0;
    FString LastId;

    UFUNCTION()
    void OnSwitched(const FString& Id)
    {
        ++HeardCount;
        LastId = Id;
    }
};
