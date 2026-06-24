// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "GTCStunnable.generated.h"

/**
 * IGTCStunnable — anything a flashbang can disable. A thrown stun (AGTCThrowable with
 * bFlashbang) finds nearby pawns and calls Stun() on those that implement this; the
 * implementer freezes its combat AI for the duration. Kept as a plain C++ interface
 * (a contract) so the throwable depends on the interface, not on concrete AI types —
 * the weapon layer never reaches up into AI.
 */
UINTERFACE(MinimalAPI)
class UGTCStunnable : public UInterface
{
    GENERATED_BODY()
};

class GTC_UE5_API IGTCStunnable
{
    GENERATED_BODY()

public:
    /** Disable combat (hold position, hold fire) for `Seconds`. */
    virtual void Stun(float Seconds) = 0;
};
