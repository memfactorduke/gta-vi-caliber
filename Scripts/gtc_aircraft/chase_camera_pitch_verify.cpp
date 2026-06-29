// Headless verifier for FVehicleChaseCamera::PitchFollow (the Wings & Waves addition).
// Compiles the REAL Camera core .cpp against the shim and checks the 3D boom-pitch math.
#include <cstdio>
#include <cmath>

#include "Math/UnrealMathUtility.h"
#include "../../Source/GTC_UE5/Camera/VehicleChaseCamera.cpp"

static int g_fail = 0;
#define CHECK(cond, msg) do { if (!(cond)) { std::printf("  FAIL: %s\n", msg); ++g_fail; } } while (0)
static float Absf(float v) { return v < 0 ? -v : v; }

int main()
{
    const float Eps = 1e-4f;
    const float Climb = std::atan2(500.0f, 1000.0f); // ~0.4636

    CHECK(Absf(FVehicleChaseCamera::PitchFollow(500.0f, 1000.0f, 1.0f, 0.6f) - Climb) < Eps, "climb -> boom tilts up");
    CHECK(Absf(FVehicleChaseCamera::PitchFollow(0.0f, 1000.0f, 1.0f, 0.6f)) < Eps, "level -> no boom pitch");
    CHECK(Absf(FVehicleChaseCamera::PitchFollow(-500.0f, 1000.0f, 1.0f, 0.6f) - (-Climb)) < Eps, "dive -> boom tilts down");
    CHECK(Absf(FVehicleChaseCamera::PitchFollow(10000.0f, 1.0f, 1.0f, 0.5f) - 0.5f) < Eps, "straight up clamps to max pitch");
    CHECK(Absf(FVehicleChaseCamera::PitchFollow(500.0f, 1000.0f, 2.0f, 2.0f) - 2.0f * Climb) < Eps, "gain scales the boom pitch");

    // Sanity: the existing speed ramps still behave (we only added a method).
    CHECK(Absf(FVehicleChaseCamera::SpeedFov(80.0f, 100.0f, 0.0f, 2000.0f) - 80.0f) < Eps, "FOV base at zero speed");
    CHECK(Absf(FVehicleChaseCamera::LookBehindYawOffset(true) - PI) < Eps, "look-behind flips 180");

    if (g_fail == 0) { std::printf("chase_camera_pitch_verify: ALL PASS\n"); return 0; }
    std::printf("chase_camera_pitch_verify: %d FAILED\n", g_fail);
    return 1;
}
