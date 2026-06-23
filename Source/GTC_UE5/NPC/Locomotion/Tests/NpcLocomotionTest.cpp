// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../NpcLocomotion.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

using GtcTest::Eps;

/**
 * Tests for FNpcLocomotion — the per-tick glue that fuses the (separately
 * parity-tested) NpcSteering / PedestrianTraffic / NpcBrain math into one desired
 * ground velocity, and owns the Godot-XZ <-> UE-ground axis bridge.
 *
 * These are NOT a Godot oracle port (no upstream equivalent — this layer is the
 * adapter the Godot project left to the engine). They pin the composition's
 * contract: the axis remap is its own inverse, Idle stands still, a clear goal is
 * sought at the state's speed, neighbours push sideways, fleeing runs away, the
 * result never exceeds the target speed, and a closing car triggers a lateral
 * dodge. Prefix GTC.NPC.Locomotion.NpcLocomotion.
 */

namespace
{
    using GtcTest::SpeedEps;
    constexpr double Walk = 150.0;
    constexpr double Run = 420.0;
}

// --- axis bridge ------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcLocomotionAxisTest,
    "GTC.NPC.Locomotion.NpcLocomotion.Axis",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcLocomotionAxisTest::RunTest(const FString& Parameters)
{
    // UE ground (X/Y, Z up) -> math plane (X/Z): UE Y becomes plane Z, UE Z dropped.
    TestTrue(TEXT("ToPlane maps UE Y to plane Z and drops UE Z"),
        FNpcLocomotion::ToPlane(FVector(1, 2, 3)).Equals(FVector(1, 0, 2), Eps));
    // Inverse: plane (X/Z) -> UE ground (X/Y), Z = 0.
    TestTrue(TEXT("FromPlane maps plane Z to UE Y, zeroes UE Z"),
        FNpcLocomotion::FromPlane(FVector(1, 0, 2)).Equals(FVector(1, 2, 0), Eps));
    // Round-trip on a ground vector is identity (the dropped UE Z was already 0).
    TestTrue(TEXT("FromPlane(ToPlane(v)) == v for a ground vector"),
        FNpcLocomotion::FromPlane(FNpcLocomotion::ToPlane(FVector(7, -4, 0)))
            .Equals(FVector(7, -4, 0), Eps));

    // GroundDistance ignores the UE up axis.
    TestTrue(TEXT("GroundDistance ignores Z"),
        FMath::IsNearlyEqual(FNpcLocomotion::GroundDistance(FVector(0, 0, 100), FVector(3, 4, 0)), 5.0, SpeedEps));
    TestTrue(TEXT("HasArrived true inside tolerance"),
        FNpcLocomotion::HasArrived(FVector(0, 0, 50), FVector(10, 0, 0), 11.0));
    TestFalse(TEXT("HasArrived false outside tolerance"),
        FNpcLocomotion::HasArrived(FVector(0, 0, 0), FVector(100, 0, 0), 11.0));

    return true;
}

// --- flee goal --------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcLocomotionFleeGoalTest,
    "GTC.NPC.Locomotion.NpcLocomotion.FleeGoal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcLocomotionFleeGoalTest::RunTest(const FString& Parameters)
{
    // Threat at +X => flee goal is planted at -X, on the NPC's own ground height.
    const FVector Goal = FNpcLocomotion::FleeGoal(FVector::ZeroVector, FVector(1000, 0, 0), 500.0);
    TestTrue(TEXT("flee goal runs directly away from threat"),
        Goal.Equals(FVector(-500, 0, 0), Eps));

    // Threat exactly on top of us => stable fallback, never NaN.
    const FVector Degenerate = FNpcLocomotion::FleeGoal(FVector(5, 5, 0), FVector(5, 5, 0), 300.0);
    TestFalse(TEXT("degenerate flee goal is finite"), Degenerate.ContainsNaN());
    TestTrue(TEXT("degenerate flee goal is 300 away"),
        FMath::IsNearlyEqual(FNpcLocomotion::GroundDistance(Degenerate, FVector(5, 5, 0)), 300.0, SpeedEps));

    return true;
}

// --- desired velocity -------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcLocomotionDesiredVelocityTest,
    "GTC.NPC.Locomotion.NpcLocomotion.DesiredVelocity",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcLocomotionDesiredVelocityTest::RunTest(const FString& Parameters)
{
    const TArray<FVector> NoNeighbors;
    const TArray<FPedestrianTraffic::FCar> NoCars;

    // Idle stands still regardless of goal.
    TestTrue(TEXT("idle yields zero velocity"),
        FNpcLocomotion::DesiredVelocity(
            FNpcBrain::EState::Idle, FVector::ZeroVector, FVector(1000, 0, 0),
            NoNeighbors, NoCars, Walk, Run) == FVector::ZeroVector);

    // Wander toward a far goal, nobody around: straight at the goal at walk speed.
    const FVector V = FNpcLocomotion::DesiredVelocity(
        FNpcBrain::EState::Wander, FVector::ZeroVector, FVector(1000, 0, 0),
        NoNeighbors, NoCars, Walk, Run);
    TestTrue(TEXT("wander heads at goal at walk speed"), V.Equals(FVector(Walk, 0, 0), SpeedEps));

    // Speed never exceeds the state's target speed (Flee => run).
    const FVector F = FNpcLocomotion::DesiredVelocity(
        FNpcBrain::EState::Flee, FVector::ZeroVector,
        FNpcLocomotion::FleeGoal(FVector::ZeroVector, FVector(300, 0, 0), 1000.0),
        NoNeighbors, NoCars, Walk, Run);
    TestTrue(TEXT("flee speed clamped to run speed"), F.Size() <= Run + SpeedEps);
    // ...and points away from the threat (threat at +X => velocity has -X component).
    TestTrue(TEXT("flee moves away from threat"), F.X < -SpeedEps);

    // A neighbour close on the +Y side pushes the wander velocity toward -Y.
    const TArray<FVector> Crowd = {FVector(0, 50, 0)};
    const FVector S = FNpcLocomotion::DesiredVelocity(
        FNpcBrain::EState::Wander, FVector::ZeroVector, FVector(1000, 0, 0),
        Crowd, NoCars, Walk, Run);
    TestTrue(TEXT("separation pushes away from neighbour"), S.Y < 0.0);
    // ...and the push is on the goal's cm/s scale, not a negligible ~1 cm/s. This
    // guards the separation units fix: a raw metres-frame push (~0.6) lost against
    // the 150 cm/s goal would only bend the path a fraction of a degree and fail here
    // (a close crowder, and the player give-way that rides this term, must visibly veer).
    TestTrue(TEXT("separation deflection is significant, not negligible"), S.Y < -10.0);

    return true;
}

// --- traffic dodge ----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcLocomotionDodgeTest,
    "GTC.NPC.Locomotion.NpcLocomotion.Dodge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcLocomotionDodgeTest::RunTest(const FString& Parameters)
{
    const TArray<FVector> NoNeighbors;

    // Baseline: heading at the goal (+X) with no cars is a pure +X velocity.
    const FVector Clear = FNpcLocomotion::DesiredVelocity(
        FNpcBrain::EState::Wander, FVector::ZeroVector, FVector(1000, 0, 0),
        NoNeighbors, {}, Walk, Run);
    TestTrue(TEXT("no-car baseline has no lateral component"), FMath::Abs(Clear.Y) < SpeedEps);

    // A car dead ahead barrelling straight back at the pedestrian: closing,
    // arrives within the horizon, zero miss distance => strong threat => the NPC
    // sidesteps, producing a lateral (UE Y) component that the clear path lacked.
    FPedestrianTraffic::FCar Car;
    Car.Pos = FVector(300, 0, 0);
    Car.Vel = FVector(-300, 0, 0);
    const FVector Dodged = FNpcLocomotion::DesiredVelocity(
        FNpcBrain::EState::Wander, FVector::ZeroVector, FVector(1000, 0, 0),
        NoNeighbors, {Car}, Walk, Run);

    TestTrue(TEXT("closing car induces a lateral dodge"), FMath::Abs(Dodged.Y) > 1.0);
    TestTrue(TEXT("dodged velocity differs from the clear path"), !Dodged.Equals(Clear, 1.0));
    TestTrue(TEXT("dodged speed still clamped to walk speed"), Dodged.Size() <= Walk + SpeedEps);

    return true;
}

// --- multi-frame simulation: it actually WALKS there over time --------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcLocomotionWalkToDestTest,
    "GTC.NPC.Locomotion.NpcLocomotion.WalksToDestination",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcLocomotionWalkToDestTest::RunTest(const FString& Parameters)
{
    // Integrate the real per-frame loop (the exact call AGTCCitizen makes each
    // tick) and confirm the citizen covers ground and arrives — proof of sustained
    // walking, not just one correct velocity sample.
    const TArray<FVector> NoNeighbors;
    const TArray<FPedestrianTraffic::FCar> NoCars;
    const double Dt = 1.0 / 30.0;
    const FVector Goal(3000, 0, 0);

    FVector Pos = FVector::ZeroVector;
    const double StartDist = FNpcLocomotion::GroundDistance(Pos, Goal);
    double PrevDist = StartDist;
    bool bMonotone = true;
    int32 Steps = 0;
    const int32 MaxSteps = 2000; // ~67s of sim — generous ceiling

    while (!FNpcLocomotion::HasArrived(Pos, Goal, 50.0) && Steps < MaxSteps)
    {
        const FVector V = FNpcLocomotion::DesiredVelocity(
            FNpcBrain::EState::Wander, Pos, Goal, NoNeighbors, NoCars, Walk, Run);
        Pos += V * Dt;
        const double D = FNpcLocomotion::GroundDistance(Pos, Goal);
        if (D > PrevDist + 1e-3) // open road: should never back away from the goal
        {
            bMonotone = false;
        }
        PrevDist = D;
        ++Steps;
    }

    TestTrue(TEXT("citizen arrives at its destination"), Steps < MaxSteps);
    TestTrue(TEXT("never walks away from the goal on open road"), bMonotone);
    TestTrue(TEXT("covered real ground"),
        FNpcLocomotion::GroundDistance(FVector::ZeroVector, Pos) > StartDist - 100.0);
    // Sanity on pacing: distance/speed gives a lower bound on the frames needed.
    const int32 MinSteps = static_cast<int32>((StartDist - 50.0) / (Walk * Dt)) - 5;
    TestTrue(TEXT("took a plausible number of steps to get there"), Steps >= MinSteps);

    return true;
}

// --- multi-frame simulation: dodges crossing traffic, then resumes ----------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FNpcLocomotionDodgeAndResumeTest,
    "GTC.NPC.Locomotion.NpcLocomotion.DodgesTrafficAndResumes",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FNpcLocomotionDodgeAndResumeTest::RunTest(const FString& Parameters)
{
    // The pedestrian heads east toward a goal; a car bears down the same lane
    // head-on, then passes and leaves behind them. Expect a real lateral sidestep
    // mid-route (perpendicular to the oncoming car), then the pedestrian recovers
    // to the travel line and still reaches the goal.
    const TArray<FVector> NoNeighbors;
    const double Dt = 1.0 / 30.0;
    const FVector Goal(3000, 0, 0);

    FVector Pos = FVector::ZeroVector;
    FVector CarPos(1600, 0, 0);
    const FVector CarVel(-260, 0, 0); // oncoming; passes the pedestrian and leaves

    double MaxLateral = 0.0;
    int32 Steps = 0;
    const int32 MaxSteps = 3000;

    while (!FNpcLocomotion::HasArrived(Pos, Goal, 60.0) && Steps < MaxSteps)
    {
        FPedestrianTraffic::FCar Car;
        Car.Pos = CarPos;
        Car.Vel = CarVel;
        const TArray<FPedestrianTraffic::FCar> Cars = {Car};

        const FVector V = FNpcLocomotion::DesiredVelocity(
            FNpcBrain::EState::Wander, Pos, Goal, NoNeighbors, Cars, Walk, Run);
        Pos += V * Dt;
        CarPos += CarVel * Dt;
        MaxLateral = FMath::Max(MaxLateral, FMath::Abs(Pos.Y));
        ++Steps;
    }

    TestTrue(TEXT("pedestrian dodged laterally while the car crossed"), MaxLateral > 30.0);
    TestTrue(TEXT("pedestrian resumed and reached the goal"), Steps < MaxSteps);
    TestTrue(TEXT("ended back near the travel line after dodging"), FMath::Abs(Pos.Y) < 80.0);

    return true;
}

#endif // WITH_AUTOMATION_TESTS
