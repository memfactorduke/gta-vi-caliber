// Copyright (c) 2026 GTC contributors

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GTCPlayerController.generated.h"

class UInputMappingContext;

/**
 * AGTCPlayerController — adds the default Enhanced Input mapping context to the
 * local player on BeginPlay.
 *
 * The IMC is soft-referenced by path (/Game/Input/IMC_Default) so the headless,
 * editor-closed build has no hard dependency on the (editor-authored) asset. The
 * IA_Move / IA_Look Axis2D composite ON IMC_Default is authored in the editor
 * and finalized when the editor reopens; this controller only installs whatever
 * IMC asset is present.
 */
UCLASS()
class GTC_UE5_API AGTCPlayerController : public APlayerController
{
    GENERATED_BODY()

public:
    AGTCPlayerController();

protected:
    virtual void BeginPlay() override;

    /** Default mapping context, soft-referenced so headless builds don't hard-link the asset. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    TSoftObjectPtr<UInputMappingContext> DefaultMappingContext;

    /** Priority for the default mapping context. */
    UPROPERTY(EditDefaultsOnly, Category = "GTC|Input")
    int32 DefaultMappingPriority = 0;
};
