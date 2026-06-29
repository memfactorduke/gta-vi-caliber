// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "GTCVehicleHudWidget.generated.h"

class AGTCKinematicVehiclePawn;

/**
 * UGTCVehicleHudWidget — the C++ base for the air/sea instrument HUD. It reads the player's
 * current vehicle and exposes the gauges as BlueprintPure readouts (via FFlightInstruments),
 * so the WBP is a thin layout that binds TextBlocks/needles to these getters — altimeter,
 * airspeed, vertical-speed and stall light for aircraft; speed + planing for boats. The
 * widget layout, the minimap hub icons, and the rotor/engine/wake Niagara are editor work;
 * the BlueprintImplementableEvent FX seams (OnExploded/OnHardImpact/OnSunk) already exist on
 * the pawns. Gate-deferred UObject; FFlightInstruments is shim-verified + GTC.* tested.
 */
UCLASS()
class GTC_UE5_API UGTCVehicleHudWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    /** True while the player is piloting a Wings & Waves vehicle (show the panel). */
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") bool IsInVehicle() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") bool IsAircraft() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") bool IsWatercraft() const;

    UFUNCTION(BlueprintPure, Category = "GTC|HUD") float AirspeedKnots() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") float AltitudeFeet() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") float ClimbFeetPerMin() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") float SpeedKmh() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") bool IsStalled() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") bool IsPlaning() const;
    UFUNCTION(BlueprintPure, Category = "GTC|HUD") float HealthFraction() const;

private:
    AGTCKinematicVehiclePawn* GetVehiclePawn() const;
};
