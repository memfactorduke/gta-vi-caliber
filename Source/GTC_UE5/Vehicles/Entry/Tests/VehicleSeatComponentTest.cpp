// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleSeatComponent.h"
#include "../VehicleEntry.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for UVehicleSeatComponent::BuildWorldSeats — the one headless-assertable
 * slice of the get-in/get-out adapter (the possess / attach / input / camera glue
 * needs a UWorld + spawned pawns, so it is DEFERRED-TO-PIE, exactly like the
 * interaction component's live gather). Pins: anchors get the FULL car transform
 * (translation + yaw); exit offsets are a DIRECTION (rotated, never translated);
 * fresh seats are unoccupied; and the built seats feed FVehicleEntry seat selection
 * so the nearest door is chosen.
 */

namespace
{
    FVehicleSeatDef MakeSeatDef(const FVector& Anchor, const FVector& Exit, bool bDriver)
    {
        FVehicleSeatDef Def;
        Def.AnchorOffset = Anchor;
        Def.ExitOffset = Exit;
        Def.bIsDriver = bDriver;
        return Def;
    }

    // Driver 45 cm LEFT (-Y), passenger 45 cm RIGHT (+Y), both 40 cm forward (+X) and
    // 20 cm up; each exits out to its own side.
    TArray<FVehicleSeatDef> TwoSeatCar()
    {
        TArray<FVehicleSeatDef> Defs;
        Defs.Add(MakeSeatDef(FVector(40, -45, 20), FVector(0, -120, -20), /*driver*/ true));
        Defs.Add(MakeSeatDef(FVector(40, 45, 20), FVector(0, 120, -20), /*driver*/ false));
        return Defs;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleSeatComponentTest,
    "GTC.Vehicles.Entry.Seat",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleSeatComponentTest::RunTest(const FString& Parameters)
{
    const TArray<FVehicleSeatDef> Defs = TwoSeatCar();

    // ---- Identity car transform: world == local ------------------------------
    {
        TArray<FVehicleEntry::FSeat> Seats;
        UVehicleSeatComponent::BuildWorldSeats(Defs, FTransform::Identity, Seats);

        TestEqual(TEXT("two seats built"), Seats.Num(), 2);
        TestTrue(TEXT("driver anchor unchanged under identity"),
            Seats[0].Anchor.Equals(FVector(40, -45, 20), Eps));
        TestTrue(TEXT("driver exit offset unchanged under identity"),
            Seats[0].ExitOffset.Equals(FVector(0, -120, -20), Eps));
        TestFalse(TEXT("freshly built seats are unoccupied"), Seats[0].bOccupied);
        // Exit STAND position is anchor + exit offset (the core's ExitStandPosition).
        TestTrue(TEXT("driver exit stand = anchor + offset"),
            FVehicleEntry::ExitStandPosition(Seats[0]).Equals(FVector(40, -165, 0), Eps));
    }

    // ---- Translated car: anchors shift; exit-offset DIRECTION does not --------
    {
        const FTransform Car(FQuat::Identity, FVector(1000, 2000, 0));
        TArray<FVehicleEntry::FSeat> Seats;
        UVehicleSeatComponent::BuildWorldSeats(Defs, Car, Seats);

        TestTrue(TEXT("anchor is translated by the car position"),
            Seats[0].Anchor.Equals(FVector(1040, 1955, 20), Eps));
        TestTrue(TEXT("exit offset (a direction) is not translated"),
            Seats[0].ExitOffset.Equals(FVector(0, -120, -20), Eps));
    }

    // ---- Yawed car (+90 deg about Z): local +X -> world +Y, local +Y -> world -X
    {
        const FTransform Car(FRotator(0, 90, 0).Quaternion(), FVector::ZeroVector);
        TArray<FVehicleEntry::FSeat> Seats;
        UVehicleSeatComponent::BuildWorldSeats(Defs, Car, Seats);

        // Driver local (40, -45, 20): X=40 -> +Y; Y=-45 -> +X 45 => (45, 40, 20).
        TestTrue(TEXT("driver anchor rotated by 90 deg yaw"),
            Seats[0].Anchor.Equals(FVector(45, 40, 20), 1.e-3));
        // Driver exit local (0, -120, -20): Y=-120 -> +X 120; Z stays => (120, 0, -20).
        TestTrue(TEXT("driver exit offset rotated by 90 deg yaw"),
            Seats[0].ExitOffset.Equals(FVector(120, 0, -20), 1.e-3));
    }

    // ---- Built seats feed seat selection: nearest door is picked --------------
    {
        TArray<FVehicleEntry::FSeat> Seats;
        UVehicleSeatComponent::BuildWorldSeats(Defs, FTransform::Identity, Seats);

        // Standing just left of the car (by the driver door) -> driver (index 0).
        TestEqual(TEXT("near the driver door picks the driver seat"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(40, -60, 20), 250.0), 0);
        // Standing just right (by the passenger door) -> passenger (index 1).
        TestEqual(TEXT("near the passenger door picks the passenger seat"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(40, 60, 20), 250.0), 1);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
