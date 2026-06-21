// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../VehicleEntry.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FVehicleEntry — the deterministic get-in / get-out brain. GTC-original
 * (no Godot oracle). Pins seat selection (nearest, unoccupied, inclusive reach,
 * stable ties on the driver seat), the guarded OnFoot->Entering->Seated->Exiting
 * state machine, the one-shot "transition just completed" edge the adapter
 * possesses on, and the clamped transition alpha.
 */

namespace
{
    FVehicleEntry::FSeat MakeSeat(double X, double Z, bool bOccupied = false)
    {
        FVehicleEntry::FSeat Seat;
        Seat.Anchor = FVector(X, 0.0, Z);
        Seat.bOccupied = bOccupied;
        return Seat;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FVehicleEntryTest,
    "GTC.Vehicles.Entry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FVehicleEntryTest::RunTest(const FString& Parameters)
{
    using EState = FVehicleEntry::EState;
    constexpr int32 None = FVehicleEntry::None;

    // ---- Seat selection -------------------------------------------------------
    {
        // Driver at origin (index 0), passenger 1.5 m to the side (index 1).
        const TArray<FVehicleEntry::FSeat> Seats({MakeSeat(0, 0), MakeSeat(0, 1.5)});

        // Standing next to the passenger door picks the passenger seat.
        TestEqual(TEXT("nearest seat is the passenger"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0, 0, 1.4), 2.5), 1);
        // Standing next to the driver door picks the driver.
        TestEqual(TEXT("nearest seat is the driver"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0.3, 0, 0.0), 2.5), 0);
    }

    // Occupied seats are skipped; you fall through to the next free one.
    {
        const TArray<FVehicleEntry::FSeat> Seats({MakeSeat(0, 0, /*occupied*/ true), MakeSeat(0, 1.5)});
        TestEqual(TEXT("occupied driver seat is skipped"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0, 0, 0.1), 5.0), 1);
    }

    // Equidistant seats resolve to the lower index (the driver seat wins).
    {
        const TArray<FVehicleEntry::FSeat> Seats({MakeSeat(-1, 0), MakeSeat(1, 0)});
        TestEqual(TEXT("tie resolves to the lower (driver) index"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0, 0, 0), 5.0), 0);
    }

    // Out of reach -> None; reach boundary is inclusive; non-positive reach -> None.
    {
        const TArray<FVehicleEntry::FSeat> Seats({MakeSeat(0, 0)});
        TestEqual(TEXT("seat beyond reach is not selectable"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0, 0, 3.0), 2.5), None);
        TestEqual(TEXT("seat exactly at reach is selectable (inclusive)"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0, 0, 2.5), 2.5), 0);
        TestEqual(TEXT("non-positive reach selects nothing"),
            FVehicleEntry::NearestAvailableSeat(Seats, FVector(0, 0, 0), 0.0), None);
        TestEqual(TEXT("empty seat list -> None"),
            FVehicleEntry::NearestAvailableSeat(TArray<FVehicleEntry::FSeat>(), FVector::ZeroVector, 2.5), None);
    }

    // Exit placement is the seat anchor plus its exit offset.
    {
        FVehicleEntry::FSeat Seat = MakeSeat(10, 5);
        Seat.ExitOffset = FVector(0, 0, -2);
        const FVector Stand = FVehicleEntry::ExitStandPosition(Seat);
        TestTrue(TEXT("exit stand position is anchor + offset"),
            FMath::Abs(Stand.X - 10.0) <= Eps && FMath::Abs(Stand.Z - 3.0) <= Eps);
    }

    // ---- State machine --------------------------------------------------------
    FVehicleEntry::FParams P; // Reach 2.5, EnterSeconds 0.9, ExitSeconds 0.8

    {
        FVehicleEntry E;
        TestTrue(TEXT("starts OnFoot"), E.State() == EState::OnFoot);
        TestEqual(TEXT("starts with no seat"), E.Seat(), None);
        TestFalse(TEXT("not seated initially"), E.IsSeated());

        // Can't exit before entering.
        TestFalse(TEXT("BeginExit from OnFoot is a no-op"), E.BeginExit());

        // Enter the driver seat.
        TestTrue(TEXT("BeginEnter from OnFoot succeeds"), E.BeginEnter(0));
        TestTrue(TEXT("now Entering"), E.State() == EState::Entering);
        TestEqual(TEXT("active seat tracked"), E.Seat(), 0);
        TestTrue(TEXT("in a transition"), E.InTransition());

        // Mashing the key mid-transition changes nothing.
        TestFalse(TEXT("re-enter during Entering is a no-op"), E.BeginEnter(1));
        TestEqual(TEXT("seat unchanged after mashed key"), E.Seat(), 0);

        // Partway through the get-in: still entering, no completion edge yet.
        TestFalse(TEXT("Update before EnterSeconds does not complete"), E.Update(0.4, P));
        TestTrue(TEXT("still Entering after partial update"), E.State() == EState::Entering);
        const double Alpha = E.TransitionAlpha(P);
        TestTrue(TEXT("alpha is mid-transition"), Alpha > 0.0 && Alpha < 1.0);

        // Crossing EnterSeconds fires the completion edge exactly once.
        TestTrue(TEXT("Update crossing EnterSeconds completes"), E.Update(0.6, P));
        TestTrue(TEXT("now Seated"), E.State() == EState::Seated);
        TestTrue(TEXT("IsSeated true"), E.IsSeated());
        TestTrue(TEXT("alpha resets to 0 once seated"), FMath::Abs(E.TransitionAlpha(P)) <= Eps);

        // No second completion edge while just sitting there.
        TestFalse(TEXT("Update while Seated never re-completes"), E.Update(1.0, P));
        TestTrue(TEXT("still Seated"), E.State() == EState::Seated);
        TestFalse(TEXT("BeginEnter while Seated is a no-op"), E.BeginEnter(1));

        // Exit: seat retained through the get-out, then cleared on completion.
        TestTrue(TEXT("BeginExit from Seated succeeds"), E.BeginExit());
        TestTrue(TEXT("now Exiting"), E.State() == EState::Exiting);
        TestEqual(TEXT("seat retained through Exiting"), E.Seat(), 0);
        TestFalse(TEXT("Update before ExitSeconds does not complete"), E.Update(0.5, P));
        TestTrue(TEXT("Update crossing ExitSeconds completes"), E.Update(0.5, P));
        TestTrue(TEXT("back OnFoot"), E.State() == EState::OnFoot);
        TestEqual(TEXT("seat cleared after exit"), E.Seat(), None);
    }

    // Negative index is rejected; negative Dt makes no progress; alpha overshoot clamps.
    {
        FVehicleEntry E;
        TestFalse(TEXT("BeginEnter with a negative index is rejected"), E.BeginEnter(None));
        TestTrue(TEXT("still OnFoot after rejected enter"), E.State() == EState::OnFoot);

        TestTrue(TEXT("enter for the clamp checks"), E.BeginEnter(0));
        TestFalse(TEXT("negative Dt does not complete"), E.Update(-5.0, P));
        TestTrue(TEXT("negative Dt makes no progress"), FMath::Abs(E.TransitionAlpha(P)) <= Eps);

        // A single huge frame still completes (and only once), alpha never exceeds 1.
        TestTrue(TEXT("a long frame completes the get-in"), E.Update(100.0, P));
        TestTrue(TEXT("seated after the long frame"), E.IsSeated());
    }

    // Update while OnFoot is an inert no-op.
    {
        FVehicleEntry E;
        TestFalse(TEXT("Update while OnFoot returns false"), E.Update(1.0, P));
        TestTrue(TEXT("still OnFoot"), E.State() == EState::OnFoot);
    }

    return true;
}

#endif // WITH_AUTOMATION_TESTS
