// Copyright Epic Games, Inc. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../PoseRig.h"
#include "../../../Tests/GtcTestTolerances.h"
#include "Math/UnrealMathUtility.h"

/**
 * Tests for FPoseRig — the posture / IK / smoothing layer that the deferred
 * Control-Rig adapter consumes. Tolerances use the shared GtcTest::Eps (1e-4
 * double); exact/structural facts assert exactly. Prefix GTC.Player.PoseRig.
 */

using GtcTest::Eps;

namespace
{
    constexpr double PoseRigHalfPi = UE_DOUBLE_HALF_PI;
    constexpr double PoseRigPi = UE_DOUBLE_PI;
    constexpr double PoseRigLn2 = 0.69314718055994530942;
}

// --- ApproachWeight ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoseRigApproachWeightTest,
    "GTC.Player.PoseRig.ApproachWeight",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoseRigApproachWeightTest::RunTest(const FString& Parameters)
{
    // Rate*Delta == ln(2) closes exactly half the gap.
    TestEqual(TEXT("half gap at ln2"),
        FPoseRig::ApproachWeight(0.0, 1.0, PoseRigLn2, 1.0), 0.5, Eps);
    // Delta <= 0 is a no-op.
    TestEqual(TEXT("zero delta no-op"),
        FPoseRig::ApproachWeight(0.3, 1.0, 10.0, 0.0), 0.3, Eps);
    // Non-positive rate is a no-op.
    TestEqual(TEXT("zero rate no-op"),
        FPoseRig::ApproachWeight(0.3, 1.0, 0.0, 0.016), 0.3, Eps);
    // Never overshoots: result stays within (current, target].
    const double Step = FPoseRig::ApproachWeight(0.0, 1.0, 8.0, 0.016);
    TestTrue(TEXT("no overshoot"), Step > 0.0 && Step < 1.0);
    // Monotonic convergence over many frames lands on the target.
    double V = 0.0;
    for (int i = 0; i < 600; ++i)
    {
        V = FPoseRig::ApproachWeight(V, 1.0, 10.0, 1.0 / 60.0);
    }
    TestTrue(TEXT("converges to target"), FMath::Abs(V - 1.0) < GtcTest::ConvergeEps);
    return true;
}

// --- SpringDamp -------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoseRigSpringDampTest,
    "GTC.Player.PoseRig.SpringDamp",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoseRigSpringDampTest::RunTest(const FString& Parameters)
{
    // Delta <= 0 leaves value and velocity untouched.
    {
        double Vel = 5.0;
        const double X = FPoseRig::SpringDamp(0.2, 1.0, Vel, 0.1, 0.0);
        TestEqual(TEXT("zero delta value"), X, 0.2, Eps);
        TestEqual(TEXT("zero delta velocity"), Vel, 5.0, Eps);
    }
    // Critically damped from rest: approaches the target from below, never past it.
    {
        double Vel = 0.0;
        double X = 0.0;
        bool bOvershot = false;
        for (int i = 0; i < 400; ++i)
        {
            X = FPoseRig::SpringDamp(X, 1.0, Vel, 0.15, 1.0 / 60.0);
            if (X > 1.0 + Eps)
            {
                bOvershot = true;
            }
        }
        TestFalse(TEXT("no overshoot"), bOvershot);
        TestTrue(TEXT("settles on target"), FMath::Abs(X - 1.0) < GtcTest::ConvergeEps);
        TestTrue(TEXT("velocity decays"), FMath::Abs(Vel) < GtcTest::ConvergeEps);
    }
    return true;
}

// --- StancePose / BlendPose -------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoseRigStancePoseTest,
    "GTC.Player.PoseRig.StancePose",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoseRigStancePoseTest::RunTest(const FString& Parameters)
{
    const FPoseRig::FStancePose Loco = FPoseRig::StancePose(FPoseRig::EPosture::Locomotion);
    const FPoseRig::FStancePose Sit = FPoseRig::StancePose(FPoseRig::EPosture::Sit);
    const FPoseRig::FStancePose Piss = FPoseRig::StancePose(FPoseRig::EPosture::Piss);

    // Neutral base carries the locomotion resting elbow bend (matches FLocomotion).
    TestEqual(TEXT("loco elbow base"), Loco.ElbowFlex, 0.35, Eps);
    TestEqual(TEXT("loco hips neutral"), Loco.HipPitch, 0.0, Eps);
    // Sitting flexes hips + knees ~90 deg and drops the pelvis.
    TestEqual(TEXT("sit hip ~90"), Sit.HipPitch, PoseRigHalfPi, Eps);
    TestEqual(TEXT("sit knee ~90"), Sit.KneeFlex, PoseRigHalfPi, Eps);
    TestTrue(TEXT("sit pelvis drops"), Sit.PelvisDrop < 0.0);
    // Relieving stance: soft knees, head down, hands gathered to front.
    TestTrue(TEXT("piss knees soft"), Piss.KneeFlex > 0.0 && Piss.KneeFlex < Sit.KneeFlex);
    TestTrue(TEXT("piss looks down"), Piss.NeckPitch > 0.0);
    TestTrue(TEXT("piss hands forward"), Piss.HandToFront > 0.5);

    // BlendPose endpoints and midpoint.
    const FPoseRig::FStancePose At0 = FPoseRig::BlendPose(Loco, Sit, 0.0);
    const FPoseRig::FStancePose At1 = FPoseRig::BlendPose(Loco, Sit, 1.0);
    const FPoseRig::FStancePose Mid = FPoseRig::BlendPose(Loco, Sit, 0.5);
    TestEqual(TEXT("blend@0 is A"), At0.HipPitch, Loco.HipPitch, Eps);
    TestEqual(TEXT("blend@1 is B"), At1.HipPitch, Sit.HipPitch, Eps);
    TestEqual(TEXT("blend@0.5 midpoint"), Mid.HipPitch, 0.5 * (Loco.HipPitch + Sit.HipPitch), Eps);
    // Weight clamps outside [0,1].
    const FPoseRig::FStancePose Over = FPoseRig::BlendPose(Loco, Sit, 2.0);
    TestEqual(TEXT("blend clamps >1"), Over.HipPitch, Sit.HipPitch, Eps);
    return true;
}

// --- SolveTwoBoneIK ---------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoseRigTwoBoneIKTest,
    "GTC.Player.PoseRig.TwoBoneIK",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoseRigTwoBoneIKTest::RunTest(const FString& Parameters)
{
    // 3-4-5 right triangle: interior angle at the middle joint is exactly PI/2.
    {
        const FPoseRig::FTwoBoneSolution S = FPoseRig::SolveTwoBoneIK(3.0, 4.0, 5.0);
        TestEqual(TEXT("3-4-5 right angle"), S.JointAngle, PoseRigHalfPi, Eps);
        TestTrue(TEXT("3-4-5 reachable"), S.bReachable);
    }
    // Fully extended: target at the sum of the bones => straight (PI), still reachable.
    {
        const FPoseRig::FTwoBoneSolution S = FPoseRig::SolveTwoBoneIK(3.0, 4.0, 7.0);
        TestEqual(TEXT("extended straight"), S.JointAngle, PoseRigPi, Eps);
        TestTrue(TEXT("extended reachable"), S.bReachable);
    }
    // Folded: target at |u-l| => joint folds shut (0).
    {
        const FPoseRig::FTwoBoneSolution S = FPoseRig::SolveTwoBoneIK(3.0, 4.0, 1.0);
        TestEqual(TEXT("folded zero"), S.JointAngle, 0.0, Eps);
        TestTrue(TEXT("folded reachable"), S.bReachable);
    }
    // Beyond reach: clamps onto the shell (straight) and reports unreachable.
    {
        const FPoseRig::FTwoBoneSolution S = FPoseRig::SolveTwoBoneIK(3.0, 4.0, 99.0);
        TestEqual(TEXT("over-reach clamps straight"), S.JointAngle, PoseRigPi, Eps);
        TestFalse(TEXT("over-reach unreachable"), S.bReachable);
    }
    // Degenerate bone length: straight + unreachable, never NaN.
    {
        const FPoseRig::FTwoBoneSolution S = FPoseRig::SolveTwoBoneIK(0.0, 4.0, 2.0);
        TestEqual(TEXT("degenerate straight"), S.JointAngle, PoseRigPi, Eps);
        TestFalse(TEXT("degenerate unreachable"), S.bReachable);
    }
    return true;
}

// --- FootPlantOffset --------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoseRigFootPlantTest,
    "GTC.Player.PoseRig.FootPlant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoseRigFootPlantTest::RunTest(const FString& Parameters)
{
    // Hip 0.9, leg 0.9 => nominal foot at 0.0. Ground at 0.1 lifts the foot 0.1.
    TestEqual(TEXT("lift to higher ground"),
        FPoseRig::FootPlantOffset(0.9, 0.1, 0.9, 0.15), 0.1, Eps);
    // Ground far above clamps to +MaxLift.
    TestEqual(TEXT("clamp high to +max"),
        FPoseRig::FootPlantOffset(0.9, 0.5, 0.9, 0.15), 0.15, Eps);
    // Ground in a dip clamps to -MaxLift.
    TestEqual(TEXT("clamp dip to -max"),
        FPoseRig::FootPlantOffset(0.9, -0.4, 0.9, 0.15), -0.15, Eps);
    // Flat ground at the nominal foot height is no offset.
    TestEqual(TEXT("flat is zero"),
        FPoseRig::FootPlantOffset(0.9, 0.0, 0.9, 0.15), 0.0, Eps);
    // Degenerate leg length is a safe zero.
    TestEqual(TEXT("zero leg safe"),
        FPoseRig::FootPlantOffset(0.9, 0.5, 0.0, 0.15), 0.0, Eps);
    return true;
}

// --- LookAtAngles -----------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FPoseRigLookAtTest,
    "GTC.Player.PoseRig.LookAt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FPoseRigLookAtTest::RunTest(const FString& Parameters)
{
    const FVector Forward(0.0, 0.0, 1.0); // facing +Z (Godot frame, Y up)

    // Dead ahead: no aim.
    {
        const FPoseRig::FAimAngles A = FPoseRig::LookAtAngles(Forward, FVector(0, 0, 1), 0.8, 0.5);
        TestEqual(TEXT("ahead yaw 0"), A.Yaw, 0.0, Eps);
        TestEqual(TEXT("ahead pitch 0"), A.Pitch, 0.0, Eps);
    }
    // Target to the right (+X) wants +PI/2 yaw but clamps to MaxYaw.
    {
        const FPoseRig::FAimAngles A = FPoseRig::LookAtAngles(Forward, FVector(1, 0, 0), 0.8, 0.5);
        TestEqual(TEXT("right yaw clamps"), A.Yaw, 0.8, Eps);
    }
    // Target up-forward wants +PI/4 pitch but clamps to MaxPitch.
    {
        const FPoseRig::FAimAngles A = FPoseRig::LookAtAngles(Forward, FVector(0, 1, 1), 0.8, 0.5);
        TestEqual(TEXT("up pitch clamps"), A.Pitch, 0.5, Eps);
    }
    // A modest off-axis target stays unclamped and signed correctly (left = -yaw).
    {
        const FPoseRig::FAimAngles A = FPoseRig::LookAtAngles(Forward, FVector(-1, 0, 1), 1.5, 1.5);
        TestEqual(TEXT("left yaw signed"), A.Yaw, -UE_DOUBLE_PI / 4.0, Eps);
    }
    // Zero-length inputs yield neutral aim, never NaN.
    {
        const FPoseRig::FAimAngles A = FPoseRig::LookAtAngles(FVector::ZeroVector, FVector(1, 0, 0), 0.8, 0.5);
        TestEqual(TEXT("zero forward yaw"), A.Yaw, 0.0, Eps);
        TestEqual(TEXT("zero forward pitch"), A.Pitch, 0.0, Eps);
    }
    return true;
}

#endif // WITH_AUTOMATION_TESTS
