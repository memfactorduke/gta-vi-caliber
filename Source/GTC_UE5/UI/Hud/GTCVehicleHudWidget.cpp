// Copyright Epic Games, Inc. All Rights Reserved.

#include "GTCVehicleHudWidget.h"

#include "FlightInstruments.h"
#include "../../Vehicles/Pawn/GTCKinematicVehiclePawn.h"
#include "../../Vehicles/Pawn/GTCVehicleLocomotionComponent.h"
#include "../../Vehicles/Aircraft/GTCAirplaneComponent.h"
#include "../../Vehicles/Aircraft/GTCHelicopterComponent.h"
#include "../../Vehicles/Watercraft/GTCBoatComponent.h"

AGTCKinematicVehiclePawn* UGTCVehicleHudWidget::GetVehiclePawn() const
{
    return Cast<AGTCKinematicVehiclePawn>(GetOwningPlayerPawn());
}

bool UGTCVehicleHudWidget::IsInVehicle() const
{
    return GetVehiclePawn() != nullptr;
}

bool UGTCVehicleHudWidget::IsAircraft() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    return Pawn != nullptr &&
        (Pawn->FindComponentByClass<UGTCAirplaneComponent>() != nullptr ||
         Pawn->FindComponentByClass<UGTCHelicopterComponent>() != nullptr);
}

bool UGTCVehicleHudWidget::IsWatercraft() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    return Pawn != nullptr && Pawn->FindComponentByClass<UGTCBoatComponent>() != nullptr;
}

float UGTCVehicleHudWidget::AirspeedKnots() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    if (Pawn == nullptr)
    {
        return 0.0f;
    }
    if (const UGTCAirplaneComponent* Plane = Pawn->FindComponentByClass<UGTCAirplaneComponent>())
    {
        return FFlightInstruments::Knots(Plane->Airspeed());
    }
    if (const UGTCVehicleLocomotionComponent* Loco = Pawn->FindComponentByClass<UGTCVehicleLocomotionComponent>())
    {
        return FFlightInstruments::Knots(Loco->WorldVelocity().Size2D());
    }
    return 0.0f;
}

float UGTCVehicleHudWidget::AltitudeFeet() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    return Pawn != nullptr ? FFlightInstruments::Feet(Pawn->GetActorLocation().Z) : 0.0f;
}

float UGTCVehicleHudWidget::ClimbFeetPerMin() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    if (Pawn == nullptr)
    {
        return 0.0f;
    }
    if (const UGTCVehicleLocomotionComponent* Loco = Pawn->FindComponentByClass<UGTCVehicleLocomotionComponent>())
    {
        return FFlightInstruments::FeetPerMin(Loco->WorldVelocity().Z);
    }
    return 0.0f;
}

float UGTCVehicleHudWidget::SpeedKmh() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    if (Pawn == nullptr)
    {
        return 0.0f;
    }
    if (const UGTCVehicleLocomotionComponent* Loco = Pawn->FindComponentByClass<UGTCVehicleLocomotionComponent>())
    {
        return FFlightInstruments::Kmh(Loco->CameraSpeed());
    }
    return 0.0f;
}

bool UGTCVehicleHudWidget::IsStalled() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    if (Pawn == nullptr)
    {
        return false;
    }
    const UGTCAirplaneComponent* Plane = Pawn->FindComponentByClass<UGTCAirplaneComponent>();
    return Plane != nullptr && Plane->IsStalled();
}

bool UGTCVehicleHudWidget::IsPlaning() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    if (Pawn == nullptr)
    {
        return false;
    }
    const UGTCBoatComponent* Boat = Pawn->FindComponentByClass<UGTCBoatComponent>();
    return Boat != nullptr && Boat->IsPlaning();
}

float UGTCVehicleHudWidget::HealthFraction() const
{
    const AGTCKinematicVehiclePawn* Pawn = GetVehiclePawn();
    return Pawn != nullptr ? Pawn->GetHealthFraction() : 0.0f;
}
