// Copyright (c) 2026 GTC contributors

#include "Misc/AutomationTest.h"

#if WITH_AUTOMATION_TESTS

#include "../GtcHudFormat.h"
#include "../../../Tests/GtcTestTolerances.h"

using GtcTest::Eps;

// ============================================================================
// GTC.UI.HudFormat — pure HUD formatting/clamp helpers. No Godot oracle (this is
// new UE-side display logic); assertions use independent, hand-checked constants.
// ============================================================================

// --- Money grouping ---------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcHudFormatMoneyTest,
    "GTC.UI.HudFormat.FormatMoney",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcHudFormatMoneyTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("zero"), GtcHud::FormatMoney(0), FString(TEXT("$0")));
    TestEqual(TEXT("under a thousand"), GtcHud::FormatMoney(750), FString(TEXT("$750")));
    TestEqual(TEXT("exactly one thousand"), GtcHud::FormatMoney(1000), FString(TEXT("$1,000")));
    TestEqual(TEXT("seeded wallet 1500"), GtcHud::FormatMoney(1500), FString(TEXT("$1,500")));
    TestEqual(TEXT("six figures"), GtcHud::FormatMoney(123456), FString(TEXT("$123,456")));
    TestEqual(TEXT("seven figures"), GtcHud::FormatMoney(1000000), FString(TEXT("$1,000,000")));
    TestEqual(TEXT("negative sign before $"), GtcHud::FormatMoney(-250), FString(TEXT("-$250")));
    TestEqual(TEXT("negative grouped"), GtcHud::FormatMoney(-12345), FString(TEXT("-$12,345")));
    // INT32_MIN magnitude overflows int32; the int64 path must still group correctly.
    TestEqual(TEXT("int32 min safe"), GtcHud::FormatMoney(MIN_int32), FString(TEXT("-$2,147,483,648")));
    return true;
}

// --- Wanted stars -----------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcHudFormatStarsTest,
    "GTC.UI.HudFormat.FormatStars",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcHudFormatStarsTest::RunTest(const FString& Parameters)
{
    TestEqual(TEXT("zero -> empty"), GtcHud::FormatStars(0), FString(TEXT("")));
    TestEqual(TEXT("three stars"), GtcHud::FormatStars(3), FString(TEXT("***")));
    TestEqual(TEXT("five stars"), GtcHud::FormatStars(5), FString(TEXT("*****")));
    TestEqual(TEXT("negative clamps to empty"), GtcHud::FormatStars(-2), FString(TEXT("")));
    // Over the cap clamps to MaxStars (default 5), never an unbounded row.
    TestEqual(TEXT("over cap clamps to five"), GtcHud::FormatStars(9), FString(TEXT("*****")));
    // Explicit cap honoured.
    TestEqual(TEXT("explicit cap of 3"), GtcHud::FormatStars(10, 3), FString(TEXT("***")));
    return true;
}

// --- Fraction clamps --------------------------------------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcHudFormatClampFractionTest,
    "GTC.UI.HudFormat.ClampFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcHudFormatClampFractionTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("mid passes through"), FMath::Abs(GtcHud::ClampFraction(0.5) - 0.5) < Eps);
    TestTrue(TEXT("zero stays zero"), FMath::Abs(GtcHud::ClampFraction(0.0)) < Eps);
    TestTrue(TEXT("one stays one"), FMath::Abs(GtcHud::ClampFraction(1.0) - 1.0) < Eps);
    TestTrue(TEXT("above one clamps to one"), FMath::Abs(GtcHud::ClampFraction(1.5) - 1.0) < Eps);
    TestTrue(TEXT("below zero clamps to zero"), FMath::Abs(GtcHud::ClampFraction(-0.25)) < Eps);
    // NaN must map to 0 (a ProgressBar fed NaN renders garbage).
    const double Nan = FMath::Sqrt(-1.0);
    TestTrue(TEXT("nan maps to zero"), FMath::Abs(GtcHud::ClampFraction(Nan)) < Eps);
    return true;
}

// --- SafeFraction (value/max with zero-max guard) ---------------------------
IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FGtcHudFormatSafeFractionTest,
    "GTC.UI.HudFormat.SafeFraction",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FGtcHudFormatSafeFractionTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("half armor"), FMath::Abs(GtcHud::SafeFraction(50.0, 100.0) - 0.5) < Eps);
    TestTrue(TEXT("full"), FMath::Abs(GtcHud::SafeFraction(100.0, 100.0) - 1.0) < Eps);
    TestTrue(TEXT("empty"), FMath::Abs(GtcHud::SafeFraction(0.0, 100.0)) < Eps);
    // Over-max clamps to 1 (e.g. overheal before the model clamps).
    TestTrue(TEXT("over max clamps to one"), FMath::Abs(GtcHud::SafeFraction(150.0, 100.0) - 1.0) < Eps);
    // Zero / negative maximum must not divide-by-zero; returns 0.
    TestTrue(TEXT("zero max safe"), FMath::Abs(GtcHud::SafeFraction(50.0, 0.0)) < Eps);
    TestTrue(TEXT("negative max safe"), FMath::Abs(GtcHud::SafeFraction(50.0, -10.0)) < Eps);
    return true;
}

#endif // WITH_AUTOMATION_TESTS
